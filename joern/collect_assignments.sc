import org.json4s.DefaultFormats
import org.json4s.native.Serialization.{read => jsonRead}
import org.json4s.native.Serialization.{write, writePretty}

// function to trace the definition of a variable
def collectAssignments(vars: List[Identifier], depth: Int, seen: Set[Call] = Set()): List[Call] = {
  if (depth == 0) return seen.toList
  val defCalls = vars
    .flatMap(_.in.dedup.hasLabel("CALL").map(_.asInstanceOf[Call]).name("<operator>.assignment").l)
    .filterNot(seen.contains)
  val nextVars = defCalls.flatMap(_.ast.isIdentifier).filterNot(v => vars.map(_.code).contains(v.code))
  collectAssignments(nextVars, depth - 1, seen ++ defCalls)
}

@main def exec(cpgFile: String, outFile: String) = {
  importCpg(cpgFile)
  implicit val formats: DefaultFormats.type = DefaultFormats

// cpg.controlStructure.controlStructureType("IF").l(2).condition.ast.isIdentifier.ddgIn.isIdentifier.ddgIn.l

  val statements = cpg.controlStructure.controlStructureType("IF")
  var output     = ""
  val lastDef    = scala.collection.mutable.ListBuffer[String]()
  for (statement <- statements) {
    val lineNumber         = statement.lineNumber
    val condition          = statement.condition
    val conditionVariables = statement.condition.ast.isIdentifier
    val assignments        = collectAssignments(conditionVariables.l, 5) // TODO: make depth configurable

    val elementMap = Map("lineNumber" -> lineNumber, "condition" -> condition.code.l)
    val json       = write(elementMap)

    output += json + "\n"
  }

  println(output)
  output |> outFile
}
