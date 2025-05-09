import org.json4s.DefaultFormats
import org.json4s.native.Serialization.{read => jsonRead}
import org.json4s.native.Serialization.{write, writePretty}


// Recursive function to trace the definition of a variable, using explicit pseudocode structure.
def recursiveDefSearch(vars: List[Identifier], depth: Int, seen: Set[CfgNode] = Set()): List[CfgNode] = {
  if (depth == 0) return seen.toList

  var defs = List[CfgNode]()
  var nextVars = List[Identifier]()

  // TODO: id.method is accessed multiple times, can we cache it?
  // TODO: Limit the CFG traversal to a specific condition statement like `if(var)`,
  // if not, the condition is "Call" and we can traverse the DDG.
  // intra-procedural to find all definitions
  for (id <- vars) {
    val idLine = id.lineNumber.getOrElse(0)
    val idName = id.name
    val assignmentCalls = id.method
      .call
      .lineNumberLte(idLine)
      .name("<operator>.assignment")
      .filter {
        c =>
          val lhs = c.argument(1)
          lhs.isIdentifier && lhs.asInstanceOf[Identifier].name == idName
      }
      .filterNot(seen.contains)
      .l
    defs ++= assignmentCalls
    nextVars ++= assignmentCalls.flatMap(_.argument(2).ast.isIdentifier.l)
    // println(s"Assignment Calls: ${assignmentCalls.code.l}")
    // println(s"Next Vars: ${nextVars.map(_.code).mkString(", ")}")

    val params = id.method
      .parameter
      .name(idName)
      .filterNot(seen.contains)
    defs ++= params
  }

  val updatedSeen = seen ++ defs
  defs ++ recursiveDefSearch(nextVars, depth - 1, updatedSeen).filterNot(defs.contains)
}

def flattenCondition(expr: Expression): List[Expression] = expr match {
  case call: Call if call.name == "<operator>.logicalOr" || call.name == "<operator>.logicalAnd" =>
    call.astChildren.collect { case e: Expression => e }.flatMap(flattenCondition).l
  case other => List(other)
}

@main def exec(cpgFile: String, outFile: String) = {
  importCpg(cpgFile)
  implicit val formats: DefaultFormats.type = DefaultFormats

  val conditions = cpg.controlStructure.controlStructureType("IF").condition.flatMap(flattenCondition)

  var output     = ""
  val lastDef    = scala.collection.mutable.ListBuffer[String]()
  for (condition <- conditions) {
    val lineNumber         = condition.lineNumber
    val conditionVariables = condition.ast.isIdentifier
    val assignments        = recursiveDefSearch(conditionVariables.l, 5) // TODO: make depth configurable

    val assignmentList = assignments.map(a => Map(
      "code" -> a.code,
      "lineNumber" -> a.lineNumber
    ))

    val elementMap = Map(
      "lineNumber" -> lineNumber,
      "condition" -> condition.code,
      "assignments" -> assignmentList
    )
    val json       = write(elementMap)

    output += json + "\n"
  }

  println(output)
  output |> outFile
}
