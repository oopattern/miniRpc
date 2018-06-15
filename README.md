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
 
 
## Performance

## Examples

Frame of phxrpc(Tencent) project, probably not correct, only for reference.
