// Callers.java <name or hex address>...: every function calling it (by symbol name match or address), decompiled
//@category PGR2
import ghidra.app.decompiler.*;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.*;
import java.util.*;

public class Callers extends GhidraScript {
    @Override
    public void run() throws Exception {
        DecompInterface d = new DecompInterface();
        d.openProgram(currentProgram);
        for (String s : getScriptArgs()) {
            List<Address> targets = new ArrayList<>();
            if (s.matches("[0-9a-fA-F]+")) targets.add(toAddr(Long.parseLong(s, 16)));
            else for (Symbol sym : currentProgram.getSymbolTable().getAllSymbols(true))
                if (sym.getName().contains(s)) { targets.add(sym.getAddress()); println("== symbol " + sym.getName() + " @ " + sym.getAddress()); }
            Set<Function> seen = new LinkedHashSet<>();
            for (Address t : targets)
                for (Reference r : getReferencesTo(t)) {
                    Function f = getFunctionContaining(r.getFromAddress());
                    println("ref from " + r.getFromAddress() + " in " + (f == null ? "?" : f.getName() + " @ " + f.getEntryPoint()));
                    if (f != null) seen.add(f);
                }
            for (Function f : seen) {
                DecompileResults res = d.decompileFunction(f, 60, monitor);
                println("== " + f.getName() + " @ " + f.getEntryPoint() + "\n" + (res.decompileCompleted() ? res.getDecompiledFunction().getC() : "failed"));
            }
        }
    }
}
