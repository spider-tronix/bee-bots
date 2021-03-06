from block import Block
import datetime
import copy
import json
import jsonpickle
import enum


class BlockChain:
    def __init__(self, data):
        self.blocks = [self.get_genesis_block(data)]

    def __getitem__(self, item):
        return self.blocks[item]

    def get_genesis_block(self, data):
        return Block(0, datetime.datetime.utcnow(), data, "arbitrary")

    def fork(self):
        c = copy.deepcopy(self)
        c.blocks = c.blocks[0 : len(self.blocks)]
        return c

    def add_block(self, data):
        chain2 = self.fork()

        block = Block(
            len(self.blocks),
            datetime.datetime.utcnow(),
            data,
            self.blocks[len(self.blocks) - 1].hash,
        )
        # if block.pos == False:
        #     print("proof of stake invalid. block wont be inserted")
        # else:
        self.blocks.append(block)
        # flag = self.verify()
        # if not flag:
        #     print("there is a conflict with the block. aborting!")
        #     self.blocks = self.blocks[0 : len(self.blocks) - 1]
        # else:
        print("Block with index " + str(len(self.blocks)) + " added")

    def get_block(self):
        return self.blocks[len(self.blocks) - 1]

    def get_chain_length(self):
        return self.blocks[len(self.blocks)]

    def verify(self):
        flag = True
        for i in range(1, len(self.blocks)):
            if self.blocks[i].index != i:
                print("wring index")
                flag = False

            if self.blocks[i - 1].hash != self.blocks[i].previous_hash:
                print("wring previous hash")
                flag = False

            # if self.blocks[i].hash != self.blocks[i].hashing():
            #     print("wrong hashing")
            #     flag = False

            if self.blocks[i - 1].timestamp >= self.blocks[i].timestamp:
                print("wring timestmp")
                flag = False
        return flag

    def toJSON(self):
        return jsonpickle.encode(self)


"""map = {
 "block_data": [
        {"id": 123, "curr_x": 123,"curr_y"=123, "final_coordinates": 123,"status" : "picked", "owner_bot_id":123}
    ],
    "bot_data": [
        {"id": 123, "block_id": 123, "curr_x": 123,"curr_y":123}
    ],
    "total_blocks": 1234,
}"""
