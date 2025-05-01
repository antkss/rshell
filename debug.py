import gdb
class DumpFreeChunks(gdb.Command):
    """Dump the free chunk linked list with colored addresses."""

    def __init__(self):
        super(DumpFreeChunks, self).__init__("aheap", gdb.COMMAND_USER)

    def invoke(self, arg, from_tty):
        YELLOW = "\033[93m"
        RESET = "\033[0m"
        RED = "\033[31m"

        pool = gdb.parse_and_eval("pool")
        free_chunk = pool['free_chunk']
        used = pool['used']
        total = pool['size']
        start = pool['start']
        free_t_type = gdb.lookup_type('free_t')

        i = 0
        print("free chunks: ")
        bins = []
        double_free = False
        double_free_chunk = 0
        while free_chunk:
            addr = int(free_chunk)
            bins.append(addr)
            size = int(free_chunk['size'])

            next_chunk = free_chunk['next']
            if free_chunk == next_chunk:
                double_free = True
                double_free_chunk = addr
                break
            # next_addr = hex(int(next_chunk)) if next_chunk else 'NULL'
            # print(f"[{i}] Chunk @ {YELLOW}{hex(addr)}{RESET} | size = {hex(size)} | next = {YELLOW}{next_addr}{RESET}")
            free_chunk = next_chunk
            i += 1
        print("allocated chunks: ")
        tmp = start
        addr_val = gdb.Value(tmp).cast(free_t_type.pointer())
        size = addr_val["size"]
        tmp_total = 0;
        i = 0
        raise_error = False
        duplicate = False
        while tmp_total < used and size != 0:
            next_chunk = tmp + size
            if next_chunk == tmp:
                duplicate = True
            if size == 0:
                print(f"{RED}error detected: chunk misaligned [size] = 0 {RESET}")
                print(f"{RED}final chunk in list: {hex(tmp)}{RESET}")
                raise_error = True
            if tmp not in bins:
                print(f"[{i}] Chunk @ {YELLOW}{hex(tmp)}{RESET} | write start: {YELLOW}{tmp + 8}{RESET} | size = {hex(size)} | next = {YELLOW}{next_chunk}{RESET}")
            else:
                print(f"[{RED}{i}{RESET}] Chunk @ {RED}{hex(tmp)}{RESET} | write start: {RED}{tmp + 8}{RESET} | size = {hex(size)} | next = {RED}{next_chunk}{RESET} (\033[31mfreed chunk{RESET})")
            tmp_total += size
            tmp = next_chunk
            addr_val = gdb.Value(tmp).cast(free_t_type.pointer())
            size = addr_val["size"]
            i += 1
        end = tmp
        if end != start + pool["used"]:
            print(f"{RED}chunks error detected [start] + [used] != [end] {RESET}")
            raise_error = True
        if double_free:
            print(f"{RED}found double free chunk in pool->free_chunk {hex(double_free_chunk)} {RESET}")
            raise_error = True
        if duplicate:
            print("error detected: chunk misaligned [next_chunk] = [tmp]")
        if not raise_error:
            print("no error, everything is aligned")

        print(f"usage {hex(used)}/{hex(total)} bytes")
        print(f"start: {YELLOW}{hex(start)}{RESET}")

DumpFreeChunks()
