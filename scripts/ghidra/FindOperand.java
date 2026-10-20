// FindOperand.java <text>...: instructions whose text contains each argument, with their functions
//@category PGR2
import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.*;

public class FindOperand extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] a = getScriptArgs();
        InstructionIterator it = currentProgram.getListing().getInstructions(true);
        while (it.hasNext()) {
            Instruction in = it.next();
            String s = in.toString();
            for (String x : a)
                if (s.contains(x)) {
                    Function f = getFunctionContaining(in.getAddress());
                    println(in.getAddress() + " " + (f == null ? "?" : f.getName()) + " : " + s);
                }
        }
    }
}
