#!/usr/bin/env python

# WS client example

import asyncio
import websockets

all_chains = []
async def main():
    count =0
    while count < 4:
        curr_chain = await websocket.recv()
        all_chains.append(curr_chain)
        
        count+=1
    uri = "ws://localhost:8765"
    async with websockets.connect(uri) as websocket:
        print(f"> running")


start_server = websockets.serve(main, "localhost", 8765)
print("running")
asyncio.get_event_loop().run_until_complete(start_server)
# query BlockChain repeatedly after set interval of time
asyncio.get_event_loop().run_forever()
