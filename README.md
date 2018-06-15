# miniRpc
Attempt to realize easy RPC

## BackGround

## Installation
 - **Compiling-Environment:**   
 Linux-2.6+  
 GCC-4.8+, support c++11  
 cmake  
 protobuf  
 
 - **Build:**  
 $ cd /path/to/miniRpc  
 $ cmake .  
 $ make   

## Features
 - **MutliThread:** one accept-thread, mutli io-thread  
 - **Coroutine:** on client mode, use coroutine fo realize RPC sync  
 - **Protobuf:** use protobuf to register RPC service, and define protocol  

## Design-Implement
 - **Framework:**  
 ![](https://github.com/oopattern/miniRpc/blob/master/screenshots/RPC.png)  

   **RpcCall/Process:** RPC business logic, refer to "rpc/rpc_channel.*, rpc/rpc_service.*"      
   **Protocol:** custom packet with protobuf, refer to "net/packet_codec.*"      
   **Network:** async epoll event implement, refer to "net/tcp_*, net/channel.*, net/acceptor.*"      
 
 - **Catalog:**  
    - base: thread operation and other common interface  
    - net : async epoll event framework, support mutli thread  
    - rpc : protobuf and coroutine rpc operation 
    - shm : mutli-process/thread lock free message queue and hash table of shm operation
    - examples : easy example of client/server demo
    - third_party: google btree library and Tencent libco library      
    
 
 
## Performance

## Examples

Frame of phxrpc(Tencent) project, probably not correct, only for reference.
