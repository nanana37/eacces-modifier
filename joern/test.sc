@main def exec(cpgFile: String, outFile: String) = {
    importCpg(cpgFile)
    cpg.method.name.l
}