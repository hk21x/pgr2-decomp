// Game code (.text) references into the static D3D device, and its calls into the D3D section.
//@category PGR2
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.*;
import java.util.*;

public class DeviceRefs extends GhidraScript {
    static final long TEXT_END = 0x1BF800, D3D_END = 0x1D2EE0, DEV = 0x1D09E0, PDEV = 0x1D09D8, DEV_SIZE = 0x24A0;
    @Override
    public void run() throws Exception {
        ReferenceManager rm = currentProgram.getReferenceManager();
        Map<Long, List<String>> devRefs = new TreeMap<>();
        Map<Long, Set<String>> calls = new TreeMap<>();
        InstructionIterator it = currentProgram.getListing().getInstructions(true);
        while (it.hasNext()) {
            Instruction in = it.next();
            long a = in.getAddress().getOffset();
            if (a >= TEXT_END) break;
            for (Reference r : in.getReferencesFrom()) {
                long t = r.getToAddress().getOffset();
                Function f = getFunctionContaining(in.getAddress());
                String fn = f == null ? String.format("nofunc@%x", a) : f.getName();
                if (t >= PDEV && t < DEV + DEV_SIZE)
                    devRefs.computeIfAbsent(t, k -> new ArrayList<>()).add(String.format("%x %s : %s", a, fn, in));
                if (r.getReferenceType().isCall() && t >= TEXT_END && t < D3D_END)
                    calls.computeIfAbsent(t, k -> new TreeSet<>()).add(fn);
            }
        }
        for (Map.Entry<Long, List<String>> e : devRefs.entrySet()) {
            println(String.format("devref +%X (%x) x%d", e.getKey() - DEV, e.getKey(), e.getValue().size()));
            for (String s : e.getValue().subList(0, Math.min(6, e.getValue().size()))) println("    " + s);
        }
        for (Map.Entry<Long, Set<String>> e : calls.entrySet()) {
            Function f = getFunctionAt(toAddr(e.getKey()));
            println(String.format("d3dcall %x %s callers=%d", e.getKey(), f == null ? "?" : f.getName(), e.getValue().size()));
        }
    }
}
