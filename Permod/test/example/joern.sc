open("fs-example.c")

cpg.assignment.code.l

// 関数 "targetFunction" 内の変数 "targetVar" の到達可能な定義を取得する

// 1. 関数を指定
val targetFunction = cpg.method("func") // 関数名を確認して修正

// 2. 変数の使用箇所を取得
val usagePoints = targetFunction.ast.isIdentifier.name("error") // 変数名を確認して修正

// 3. 各使用箇所に対して到達可能な定義を取得
usagePoints.foreach { usage =>
  println(s"Usage of variable 'error' at line ${usage.lineNumber.getOrElse("N/A")}:")
  
  // データフロー解析を使用して到達可能な定義を取得
  val definitions = usage.reachableBy(cpg.assignment).l

  // 定義が見つからない場合のデバッグ出力
  if (definitions.isEmpty) {
    println("No reachable definitions found. Check the variable name or assignment nodes.")
  }

  // 定義を出力
  definitions.foreach { definition =>
    println(s"  - Defined as: ${definition.code} (at line ${definition.lineNumber.getOrElse("N/A")})")
  }
  println("-" * 50)
}