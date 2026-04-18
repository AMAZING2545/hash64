#include <iostream>
#include <vector>
#include <cstdint>
#include <iomanip>
#include <random>
#include "json.hpp"
#include "httplib.h"
#include <fstream>
#include <sstream>
#include "cryptography.cpp"
#include "blockchain.cpp"

#define CPPHTTPLIB_OPENSSL_SUPPORT

int main() {
    srand(time(nullptr));
    blockchain bc;
    string address;
    cout<<"enter node address ";
    cin>>address;
    httplib::Client cli(address);
    cout<<"hello!!"<<endl;
    uint64_t lh=0;
    while(true){
      std::this_thread::sleep_for(std::chrono::milliseconds(10000));
      auto canmine = cli.Get("/canmine");
      if(!canmine){
        cerr<<"failed to connect\n";
        continue;
      }
      if(canmine->body=="1"){
       auto res = cli.Get("/block_transactions");
      json j0 = json::parse(res->body);
      for (const auto& tx_json : j0["t"]) {
                transaction trans;
                trans.number = tx_json["number"];
                trans.sender.parts[0] = tx_json["s0"];
                trans.sender.parts[1] = tx_json["s1"];
                trans.sender.parts[2] = tx_json["s2"];
                trans.sender.parts[3] = tx_json["s3"];
                trans.sender.parts[4] = tx_json["s4"];
                trans.receiver.parts[0] = tx_json["r0"];
                trans.receiver.parts[1] = tx_json["r1"];
                trans.receiver.parts[2] = tx_json["r2"];
                trans.receiver.parts[3] = tx_json["r3"];
                trans.receiver.parts[4] = tx_json["r4"];
                trans.amount = tx_json["amount"];
                trans.signature[0] = tx_json["sig0"];
                trans.signature[1] = tx_json["sig1"];
                trans.signature[2] = tx_json["sig2"];
                trans.signature[3] = tx_json["sig3"];
                trans.signature[4] = tx_json["sig4"];
                trans.signature[5] = tx_json["sig5"];
                trans.signature[6] = tx_json["sig6"];
                trans.signature[7] = tx_json["sig7"];
                bc.pending_transactions.push_back(trans);
      }
      bc.difficulty=j0["d"];
      do{
        bc.chain.clear();
        auto res1 = cli.Get("/last_block");
        if(res1){
          block b;
          json j1 = json::parse(res1->body);
          b.hash = j1["hash"];
          b.height = j1["height"];
          bc.chain.push_back(b);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10000));
      }while(bc.chain.back().height==lh);
      lh = bc.chain.back().height;
      cout<<"mining\n";
      bc.mine_new(time(nullptr), {17696393948516182787ULL,
      13902621260475358775ULL,
      15621036470522131904ULL,
      9159884703671929289ULL,
      242ULL}); //put your public key here
      bc.pending_transactions.clear();
            block b = bc.chain.back();
            json block_json;
            block_json["previous_hash"] = b.previous_hash;
            block_json["timestamp"] = b.timestamp;
            block_json["block_reward_receive"] = b.block_reward_receive;
            
            block_json["r0"] = b.block_reward_address.parts[0];
            block_json["r1"] = b.block_reward_address.parts[1];
            block_json["r2"] = b.block_reward_address.parts[2];
            block_json["r3"] = b.block_reward_address.parts[3];
            block_json["r4"] = b.block_reward_address.parts[4];
                
            block_json["nonce"] = b.nonce;
            block_json["extended_nonce"] = b.nonce_2;
            block_json["hash"] = b.hash;
            block_json["height"] = b.height;
            json tx_array = json::array();
            for (const auto& trans : b.transactions) {
                json tx_json;
                tx_json["number"] = trans.number;
                tx_json["s0"] = trans.sender.parts[0];
                tx_json["s1"] = trans.sender.parts[1];
                tx_json["s2"] = trans.sender.parts[2];
                tx_json["s3"] = trans.sender.parts[3];
                tx_json["s4"] = trans.sender.parts[4];
                tx_json["r0"] = trans.receiver.parts[0];
                tx_json["r1"] = trans.receiver.parts[1];
                tx_json["r2"] = trans.receiver.parts[2];
                tx_json["r3"] = trans.receiver.parts[3];
                tx_json["r4"] = trans.receiver.parts[4];
                tx_json["amount"] = trans.amount;
                tx_json["sig0"] = trans.signature[0];
                tx_json["sig1"] = trans.signature[1];
                tx_json["sig2"] = trans.signature[2];
                tx_json["sig3"] = trans.signature[3];
                tx_json["sig4"] = trans.signature[4];
                tx_json["sig5"] = trans.signature[5];
                tx_json["sig6"] = trans.signature[6];
                tx_json["sig7"] = trans.signature[7];
                tx_array.push_back(tx_json);
            }
            block_json["transactions"] = tx_array;
            string response = block_json.dump();
            auto res2 = cli.Post("/send_block",response ,"text/plain");
      }
    }
    
    
    return 0;
}
