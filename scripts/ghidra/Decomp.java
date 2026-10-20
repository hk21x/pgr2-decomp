// Decompiles the functions at the addresses in script args and lists their callers.
//@category PGR2
import ghidra.app.decompiler.*;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.*;

public class Decomp extends GhidraScript {
    @Override
    public void run() throws Exception {
        DecompInterface d = new DecompInterface();
        d.openProgram(currentProgram);
        for (String s : getScriptArgs()) {
            Function f = getFunctionContaining(toAddr(Long.parseLong(s, 16)));
            if (f == null) { println("== " + s + " no function"); continue; }
            StringBuilder sb = new StringBuilder("== " + f.getName() + " @ " + f.getEntryPoint() + "\ncallers:");
            for (Reference r : getReferencesTo(f.getEntryPoint())) {
                Function c = getFunctionContaining(r.getFromAddress());
                sb.append(" ").append(r.getFromAddress()).append("(").append(c == null ? "?" : c.getName()).append(")");
            }
            DecompileResults res = d.decompileFunction(f, 60, monitor);
            sb.append("\n").append(res.decompileCompleted() ? res.getDecompiledFunction().getC() : "decompile failed");
            println(sb.toString());
        }
    }
}
