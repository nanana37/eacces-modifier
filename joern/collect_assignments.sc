import org.json4s.DefaultFormats
import org.json4s.native.Serialization.{read => jsonRead}
import org.json4s.native.Serialization.{write, writePretty}


// Recursive function to trace the definition of a variable, using explicit pseudocode structure.
def recursiveDefSearch(vars: List[Identifier], depth: Int, seen: Set[CfgNode] = Set()): List[CfgNode] = {
  if (depth == 0) return seen.toList

  var defs = List[CfgNode]()
  var nextVars = List[Identifier]()

  for (v <- vars) {
    val ddgIns = v.ddgIn.l
    val astIdentifiers = ddgIns.flatMap(_.ast.isIdentifier.l)

    // Collect assignments
    val assignmentCalls = v.astParent.collect {
      case c: Call if c.name == "<operator>.assignment" => c
    }.filterNot(seen.contains)
    defs ++= assignmentCalls
    // println(s"Assignment Calls: ${assignmentCalls.code.l}")

    // Collect parameters
    val params = ddgIns.hasLabel("METHOD_PARAMETER_IN").filterNot(seen.contains)
    defs ++= params
    // println(s"Parameters: ${params.code.l}")

    nextVars ++= astIdentifiers
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
