# hash64
a PoW blockchain based cryptocurrency written in C++\
\
get the latest node binaries in the release tab\
\
HTTP endpoints:\
  GET http://yourip:8080/keypair \
    create a new public/private keypair\
  GET http://yourip:8080/balance?q=public_key \
    get the balance of that wallet address\
  GET http://yourip:8080/validate_chain \
    validate current blockchain status\
  POST http://yourip:8080/new_transaction \
    send a new transaction to the server as JSON\
  POST http://yourip:8080/send_block \
    send your mined block to the server as JSON\
  GET http://yourip:8080/last_block \
    get the latest block in the blockchain\
  GET http://yourip:8080/pending_transactions \
    get the current list of pending transactions\

discuss on https://www.reddit.com/r/hash64

here is the genesis block:\
        {\
            "block_reward_receive": 524288,\
            "extended_nonce": 0,\
            "hash": 584540248396,\
            "height": 0,\
            "nonce": 23064795,\
            "previous_hash": 0,\
            "r0": 3561335371994406147,\
            "r1": 3980349921941946895,\
            "r2": 1669028706157324894,\
            "r3": 352767835231390468,\
            "r4": 119,\
            "timestamp": 1775941331,\
            "transactions": []\
        }\

this project uses httplib.h and json.hpp, which are licensed under the MIT license
