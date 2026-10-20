// Decompiles every function whose entry is in [args0, args1) to the file args2, with callers.
//@category PGR2
import ghidra.app.decompiler.*;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.*;
import java.io.*;

public class DumpRange extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] a = getScriptArgs();
        long lo = Long.parseLong(a[0], 16), hi = Long.parseLong(a[1], 16);
        DecompInterface d = new DecompInterface();
        d.openProgram(currentProgram);
        try (PrintWriter w = new PrintWriter(new FileWriter(a[2]))) {
            for (Function f : currentProgram.getFunctionManager().getFunctions(toAddr(lo), true)) {
                if (f.getEntryPoint().getOffset() >= hi) break;
                StringBuilder sb = new StringBuilder("\n######## " + f.getName() + " @ " + f.getEntryPoint() + " size " + f.getBody().getNumAddresses() + "\ncallers:");
                int n = 0;
                for (Reference r : getReferencesTo(f.getEntryPoint())) {
                    if (n++ > 40) { sb.append(" ..."); break; }
                    Function c = getFunctionContaining(r.getFromAddress());
                    sb.append(" ").append(r.getFromAddress()).append("(").append(c == null ? "?" : c.getName()).append(")");
                }
                DecompileResults res = d.decompileFunction(f, 60, monitor);
                sb.append("\n").append(res.decompileCompleted() ? res.getDecompiledFunction().getC() : "decompile failed");
                w.println(sb);
            }
        }
    }
}
