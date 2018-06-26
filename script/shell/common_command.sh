
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
Network operation(tcpdump/netstat/lsof/nc...)

1) collect network packet, use with wireshark:
    # tcpdump -i lo tcp port 8888 -X -s 0 -v
2) show packet detail message:
    # tcpdump -i eth0 host 192.168.24.126  and dst port 4502 -n -c 300 -X -s 0 | grep "length 10[1-3]" -A 5
3) show connection
    # netstat -antp | grep 7050
4) show fd from process
    # lsof -p 10454
    # lsof -i:port
    # lsof -Pan -i tcp |grep LISTEN | egrep "10050|6133"


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
Memory operation(free/top/ps)

1) show system memory
    # free -m
2) show system run process
    # top -p PID
    # top, (print 1: show all cpu, print M: sort from memory, print C: sort from cpu)
3) show special run process
    # cat /proc/PID/status
4) show process info
    # ps aux | grep PID


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
GDB operation

1) gdb process_name -p PID
2) b class::func_name, (set breakpoint for debug)
3) info b, (look breakpoint info)
4) info local, (look local variable)
5) r, (run process)
6) c, (continue, when stop at breakpoint)
7) s, (go next statement, will go into func)
8) n, (go next statement, go out from func)
9) p *this, (show variable info)  
   p xxx, (xxx is a variable)
10)bt, (show stack overflow info)
11)f xxx, (xxx is line number from bt command)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
Text operation(grep/awk/sed)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
Share memory operation(ipcs/ipcrm)

1) SystemV IPC:
    # ipcs
    # ipcrm -m shmid
2) Posix IPC:
    # ll /dev/shm/xxx, xxx is shm_open object for share memory


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
Other operation
1) generate proto file, such as *.pb.cc and *.pb.h files
    # protoc --cpp_out=./ std_rpc_meta.proto, you can refer to CMakeLists.txt
2) try mount command to share file between windows and ubuntu
    # mount -t vboxsf Github /mnt/share
