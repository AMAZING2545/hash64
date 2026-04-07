#include <iostream>
#include <vector>
#include <cstdint>
#include <iomanip>
#include <random>
#include "json.hpp"
#include "httplib.h"
#include <fstream>
#include <sstream>
using json = nlohmann::json;

using namespace std;


struct transaction{
  uint64_t number;
  uint64_t sender;
  uint64_t receiver;
  uint64_t amount;
  uint64_t hash;
};

uint64_t barrel_leftshift(uint64_t y, uint64_t a) {
    a = a & 63;
    y = (y << a) | (y >> (sizeof(y)*8 - a));
    return y;
}

uint64_t tinyhash(const vector<uint64_t>& v) {
    uint64_t sum = 0;
    uint64_t guess = v.at(0);
    uint64_t secret = 0x9f4289e276d334a0;
    int size = v.size();
    
    for (auto &i : v) 
        sum = sum + i;
    
    for (int i = 0; i < size; ++i) {
        guess = guess + barrel_leftshift(sum, v.at(i));
        guess = barrel_leftshift(guess, sum - i);
        guess = guess xor barrel_leftshift(secret, v.at(i) + guess);
    }
    
    return guess;
}

uint64_t get_public(uint64_t private_key){
  uint64_t a=private_key;
  for(int i = 0; i<6; i++) a*=private_key%17446744073709551611ULL;
  return a;
}



class block{
private:
  
  uint16_t size = 6;

public:
  vector<uint64_t>vec;
  uint64_t height;
  uint64_t previous_hash;
  uint64_t timestamp;
  vector<transaction> transactions;
  uint64_t block_reward_receive;
  uint64_t block_reward_address;
  uint64_t nonce=0;
  uint64_t nonce_2=0;
  uint64_t hash;
  
  vector<uint64_t> vectorize(){
    transaction tmp;
    size = 6;
    size+=transactions.size()*5;
    vec.clear();
    vec.reserve(size);
    vec.push_back(height);
    vec.push_back(previous_hash);
    vec.push_back(timestamp);
    for(uint16_t j = 0;j<transactions.size();j++){
      tmp=transactions.at(j);
      vec.push_back(tmp.number);
      vec.push_back(tmp.sender);
      vec.push_back(tmp.receiver);
      vec.push_back(tmp.amount);
      vec.push_back(tmp.hash);
    }
    vec.push_back(block_reward_receive);
    vec.push_back(block_reward_address);
    vec.push_back(nonce_2);
    vec.push_back(nonce);
    
    
    return vec;
  }
  void print_vectorization(const vector<uint64_t>& v) {
    cout << "Vectorizing " << v.size() << " values:\n";
    for (size_t i = 0; i < v.size() && i < 20; i++) {
        cout << "  [" << i << "] = " << v[i] << "\n";
      }
  }
  uint64_t calculate_hash(){
    return  tinyhash(vec);
  }
  bool isvalid(uint64_t difficulty){
    vec=vectorize();
    return (hash<difficulty)&&(calculate_hash()==hash);
  }
  void mine(uint64_t difficulty){
    vec=vectorize();
    while(tinyhash(vec)>difficulty){
      nonce = 0;
      vec=vectorize();
      while(vec.back()<0xffffffffffffffffULL){
        vec.back()++;
        if(tinyhash(vec)<=difficulty) goto exit;
      }
      nonce_2++;
    }
    exit:
    hash=calculate_hash();
    nonce=vec.back();
    cout<<"block mined! hash = "<<hex<<hash<<"\nnonce = "<<nonce_2<<nonce;
    print_vectorization(vec);
  }
};










void to_json(json& j, const transaction& t) {
    j = json{
        {"number", t.number},
        {"sender", t.sender},
        {"receiver", t.receiver},
        {"amount", t.amount},
        {"hash", t.hash}
    };
}

void from_json(const json& j, transaction& t) {
    j.at("number").get_to(t.number);
    j.at("sender").get_to(t.sender);
    j.at("receiver").get_to(t.receiver);
    j.at("amount").get_to(t.amount);
    j.at("hash").get_to(t.hash);
}

void to_json(json& j, const block& b) {
    j = json{
        {"height", b.height},
        {"previous_hash", b.previous_hash},
        {"timestamp", b.timestamp},
        {"transactions", b.transactions},
        {"block_reward_receive", b.block_reward_receive},
        {"block_reward_address", b.block_reward_address},
        {"nonce", b.nonce},
        {"hash", b.hash}
    };
}

void from_json(const json& j, block& b) {
    j.at("height").get_to(b.height);
    j.at("previous_hash").get_to(b.previous_hash);
    j.at("timestamp").get_to(b.timestamp);
    j.at("transactions").get_to(b.transactions);
    j.at("block_reward_receive").get_to(b.block_reward_receive);
    j.at("block_reward_address").get_to(b.block_reward_address);
    j.at("nonce").get_to(b.nonce);
    j.at("hash").get_to(b.hash);
}














class blockchain{

public:
  uint64_t height=0;
  uint64_t trans_number;
  vector<block> chain;
  vector<transaction> pending_transactions;
  uint64_t difficulty=2000000000;
  uint64_t reward=1000;
  uint64_t miners=2;
  blockchain(){
    pending_transactions.reserve(100);
    cout<<"created new blockchain!\n";
  }
  void genesisblock(uint64_t timestamp){
    block genesis;
    genesis.height=0;
    genesis.timestamp=timestamp;
    genesis.previous_hash=0;
    genesis.transactions={};
    genesis.block_reward_receive=10;
    genesis.block_reward_address=0;
    genesis.mine(difficulty);
    chain.push_back(genesis);
  }
  void add_transaction(transaction trans){
    pending_transactions.push_back(trans);
    cout<<"queue is "<<pending_transactions.size()<<" long\n";
  }
  block get_last_block(){
    return chain.back(); 
  }
  uint64_t get_balance(uint64_t address, bool &valid){
    //this is tricky because it requires unwinding the blockchain
    
    block b;
    transaction tr;
    uint64_t balance=0;
    int i;
    for(i = 0; i<chain.size(); i++){
      b=chain.at(i);
      vector<transaction> t=b.transactions;
      if(b.block_reward_address==address)
        balance+=b.block_reward_receive;
      for(int j = 0; j<t.size(); j++){
        tr=t.at(j);
        if(tr.sender==address){
          if(balance < tr.amount){
            cout<<"!negative balance!";
            valid=0;
            return 0;
          }
          balance-=tr.amount;
        }
        if(tr.receiver==address) balance+=tr.amount;
      }
    }
    valid= 1;
    return balance;
  }
  void mine_new(uint64_t timestamp, uint64_t address){
    block b,l=get_last_block();
    height++;
    b.height=height;
    b.timestamp=timestamp;
    b.previous_hash=l.hash;
    b.transactions=pending_transactions;
    pending_transactions.clear();
    pending_transactions.reserve(100);
    b.block_reward_receive=reward;
    b.block_reward_address=address;
    b.mine(difficulty);
    chain.push_back(b);
  }
  bool send_block(block b){
    if(b.isvalid(difficulty)){
      chain.push_back(b);
      return 0;
    }
    return 1;
  }
  transaction new_transaction(uint64_t private_key, uint64_t receiver, uint64_t amount, bool ignore){
    transaction trans;
    trans.number=time(nullptr);
    trans.sender=get_public(private_key);
    trans.receiver=receiver;
    trans.amount=amount;
    bool valid;
    int64_t balance = get_balance(trans.sender, valid);
    if (!valid) trans.amount=0;
    if (balance<trans.amount&&!ignore){
      cout<<"you do not have enough money "<<balance<<endl;
      trans.amount=0;
    }
    if (receiver == trans.sender){
      cout<<"no dupe here, you fool\n";
      trans.amount=0;
    }
    vector<uint64_t> v ={trans.number,trans.sender,receiver,amount};
    trans.hash=tinyhash(v)|(private_key&17769786425166460570ULL);
    return trans;
  }
  
  
  
  

  
    bool save_to_file(const string& filename) {
        json j;
        
        json chain_array = json::array();
        for (const auto& b : chain) {
            json block_json;
            block_json["previous_hash"] = b.previous_hash;
            block_json["timestamp"] = b.timestamp;
            block_json["block_reward_receive"] = b.block_reward_receive;
            block_json["block_reward_address"] = b.block_reward_address;
            block_json["nonce"] = b.nonce;
            block_json["extended_nonce"] = b.nonce_2;
            block_json["hash"] = b.hash;
            block_json["height"] = b.height;
            json tx_array = json::array();
            for (const auto& trans : b.transactions) {
                json tx_json;
                tx_json["number"] = trans.number;
                tx_json["sender"] = trans.sender;
                tx_json["receiver"] = trans.receiver;
                tx_json["amount"] = trans.amount;
                tx_json["hash"] = trans.hash;
                tx_array.push_back(tx_json);
            }
            block_json["transactions"] = tx_array;
            
            chain_array.push_back(block_json);
        }
        
        j["chain"] = chain_array;
        j["difficulty"] = difficulty;
        j["reward"] = reward;
        j["height"]=height;
        j["miners"]=miners;
        json pending_array = json::array();
        for (const auto& tx : pending_transactions) {
            json tx_json;
            tx_json["number"] = tx.number;
            tx_json["sender"] = tx.sender;
            tx_json["receiver"] = tx.receiver;
            tx_json["amount"] = tx.amount;
            tx_json["hash"] = tx.hash;
            pending_array.push_back(tx_json);
        }
        j["pending_transactions"] = pending_array;
        
        // Write to file
        ofstream file(filename);
        if (!file.is_open()) {
            cerr << "Failed to open file for writing: " << filename << endl;
            return false;
        }
        
        file << setw(4) << j << endl;
        file.close();
        
        cout << "Blockchain saved to " << filename << " (" << chain.size() 
             << " blocks, " << pending_transactions.size() << " pending tx)\n";
        
        // Debug: Print first block's transaction count
        if (!chain.empty() && !chain[0].transactions.empty()) {
            cout << "First block has " << chain[0].transactions.size() << " transactions\n";
        }
        
        return true;
    
    }
    bool load_from_file(const string& filename) {
      ifstream file(filename);
      if (!file.is_open()) {
          cout << "failed to open file: "<<filename <<endl;
          return false;
      }
        
      json j;
      file >>j;
      file.close();
      chain.clear();
      pending_transactions.clear();
        
      if (j.contains("chain") && j["chain"].is_array()) {
        for (const auto& block_json : j["chain"]) {
          block b;
          b.previous_hash = block_json["previous_hash"];
          b.timestamp = block_json["timestamp"];
          b.block_reward_receive = block_json["block_reward_receive"];
          b.block_reward_address = block_json["block_reward_address"];
          b.hash = block_json["hash"];
          b.height=block_json["height"];
          b.nonce=block_json["nonce"];
          b.nonce_2=block_json["extended_nonce"];
          if (block_json.contains("transactions")&&block_json["transactions"].is_array()) {
              for (const auto& tx_json : block_json["transactions"]) {
                transaction trans;
                trans.number = tx_json["number"];
                trans.sender = tx_json["sender"];
                trans.receiver = tx_json["receiver"];
                trans.amount = tx_json["amount"];
                trans.hash = tx_json["hash"];
                b.transactions.push_back(trans);
              }
          }
                
          chain.push_back(b);
        }
      }
        
      difficulty = j["difficulty"];
      reward = j["reward"];
      height =j["height"];
      miners =j["miners"];
      for (const auto& tx_json : j["pending_transactions"]) {
        transaction trans;
        trans.number = tx_json["number"];
        trans.sender = tx_json["sender"];
        trans.receiver = tx_json["receiver"];
        trans.amount = tx_json["amount"];
        trans.hash = tx_json["hash"];
        pending_transactions.push_back(trans);
      }
        
        
        return true;
        
    }

    
    bool append_block_to_file(const string& filename, const block& new_block) {
        blockchain temp;
        if (temp.load_from_file(filename)) {
            temp.chain.push_back(new_block);
            return temp.save_to_file(filename);
        }
        return 0;
    }

    bool is_chain_valid() {
      if (chain.empty()) {
        cout << "empty" << endl;
        return 0;
      }
      if (chain.at(0).previous_hash != 0) {
        cout << "invalid genesis block: previous_hash should be 0"<< endl;
        return 0;
      }
      for (size_t i = 0; i < chain.size(); i++) {
        block& current = chain.at(i);
        uint64_t hash = current.hash;
        current.vec=current.vectorize();
        current.print_vectorization(current.vec);
        if (hash != current.calculate_hash()) {
          cout<<"block " << i << " hash mismatch! Stored: " <<dec<< hash<<" calculated: "<< current.calculate_hash()<< endl;
          return 0;
        }
        
        if (i > 0) {
          if (current.previous_hash != chain.at(i-1).hash) {
            cout<< "block "<<i<<" previous hash mismatch! expected: "<< dec<< chain.at(i-1).hash <<" got: "<<current.previous_hash<< endl;
            return 0;
          }
        }
        if (hash >= difficulty) {
            cout<< "block "<<i<<" does not meet difficulty requirements! hash: "<<dec<<hash
            <<"\ndifficulty: "<<difficulty<<endl;
            return 0;
        }

      }
      cout << "validation passed " << chain.size() << "blocks are valid." << endl;
      return 1;
    }
};

uint64_t get_private(){
  uint64_t t = time(nullptr);
  vector<uint64_t> p = {t};
  return tinyhash(p);
}

int main() {
    blockchain bc;
    vector<block> buffer;
    vector<unsigned long long> hash_buffer;
    int submissions=0;
    const string save_file = "chain.json";
    
    ifstream test(save_file);
    if(test.good()) {
      test.close();
      cout << "loading\n";
      if (!bc.load_from_file(save_file)) {
        cout << "creating genesis block\n";
        bc.genesisblock(time(nullptr));
      }
    }
    else {
      cout << "creating genesis block\n";
      bc.genesisblock(time(nullptr));
    }
    
    
    
    
    httplib::Server server;
    
    server.set_keep_alive_max_count(5);
    server.set_read_timeout(5, 0);   // 5 seconds
    server.set_write_timeout(5, 0);
    server.set_idle_interval(0, 100000); // 100ms
    
    server.Get("/keypair", [](const auto &req, auto &res) {
          res.status = 200;
          uint64_t p = get_private();
          std::string response = "private key = ";
          response += std::to_string(p);
          response += "\npublic key = ";
          response += std::to_string(get_public(p));
          res.set_content(response, "text/plain");
    });
    server.Get("/validate_chain", [&bc](const auto &req, auto &res) {
          res.status = 200;
          string response;
          if(bc.is_chain_valid())
            response="valid";
          else
            response="invalid";
          res.set_content(response, "text/plain");
    });
    server.Get("/balance", [&bc](const auto &req, auto &res) {
          auto q = req.get_param_value("q");
          uint64_t a = stoi(q);
          std::string response = "balance = ";
          bool v;
          response += std::to_string(bc.get_balance(a, v));
          res.status = 200;
          res.set_content(response, "text/plain");
    });
    server.Post("/new_transaction", [&bc](const auto &req, auto &res) {
          json tx_json = json::parse(req.body);
          transaction trans;
          trans.number = tx_json["number"];
          trans.sender = tx_json["sender"];
          trans.receiver = tx_json["receiver"];
          trans.amount = tx_json["amount"];
          trans.hash = tx_json["hash"];
          bool valid=0;
          string response;
          bc.get_balance(trans.sender, valid);
          if(!valid)
            response="invalid transaction";
          else{
            response="transaction submitted, waiting for approval";
            bc.pending_transactions.push_back(trans);
          }
          res.status = 200;
          res.set_content(response, "text/plain");
    });
    server.Post("/send_block", [&buffer, &hash_buffer, &bc, &submissions](const auto &req, auto &res) {
          block b;
          json block_json=json::parse(req.body);
          b.previous_hash = block_json["previous_hash"];
          b.timestamp = block_json["timestamp"];
          b.block_reward_receive = block_json["block_reward_receive"];
          b.block_reward_address = block_json["block_reward_address"];
          b.hash = block_json["hash"];
          b.height=block_json["height"];
          b.nonce=block_json["nonce"];
          b.nonce_2=block_json["extended_nonce"];
          if (block_json.contains("transactions")&&block_json["transactions"].is_array()) {
              for (const auto& tx_json : block_json["transactions"]) {
                transaction trans;
                trans.number = tx_json["number"];
                trans.sender = tx_json["sender"];
                trans.receiver = tx_json["receiver"];
                trans.amount = tx_json["amount"];
                trans.hash = tx_json["hash"];
                b.transactions.push_back(trans);
              }
          }
          string response;
          b.print_vectorization(b.vectorize());
          cout<<"submissions: "<<submissions<<endl;
          cout<<dec<<tinyhash(b.vectorize())<<flush;
          bool valid=tinyhash(b.vectorize())==b.hash;
          if(!b.transactions.empty()){
          for (const auto& tx : b.transactions) {
            uint64_t balance = bc.get_balance(tx.sender, valid);
            if (!valid) 
              break;
          }
          }
          if(!valid){
             response = "INVALID BLOCK, rejected";
          }
          else{
            response = "VALID, waiting for verification";
            buffer.push_back(b);
            uint64_t tmp_time = b.timestamp, tmp_nonce=0;
            b.timestamp=0;
            b.nonce=0;
            vector<uint64_t> vc;
            vc=b.vectorize();
            hash_buffer.push_back(tinyhash(vc));
            b.timestamp=tmp_time;
            b.nonce=tmp_nonce;
            submissions++;
            if (submissions==bc.miners/2){
              int counter=0;
                //check if hashes match
                uint64_t n = hash_buffer.size(), maxcount = 0;
                uint64_t res;
    
                for (int i = 0; i < n; i++) {
                    uint64_t count = 0;
                    for (int j = 0; j < n; j++) {
                      if (hash_buffer[i] == hash_buffer[j])
                        count++;
                    }
                              
                    // If count is greater or if count 
                    // is same but value is bigger.
                    if (count > maxcount || (count == maxcount && hash_buffer[i] > res)) {
                       maxcount = count;
                       res = hash_buffer[i];
                    }
               }
              //res contains the most common
              //linear search
              uint64_t prev_time=0;
              uint64_t majority_ix=0;
              bool valid=1;
              for(int i = 0; i<n;i++){
                if(prev_time==buffer.at(i).timestamp){
                  cout<<"same timestamp"<<endl;
                  valid=0;
                }
                if(hash_buffer.at(i)==res){
                  prev_time=buffer.at(i).timestamp;
                  majority_ix=i;
                }
              }
              if(!valid){
                response="VERIFICATION FAILED";
              }
              else{
                response="PASSED, appending to blockchain";
                bc.chain.push_back(buffer.at(majority_ix));
                bc.save_to_file("chain.json");
              }
              buffer.clear();
              hash_buffer.clear();
              bc.pending_transactions.clear();
          }
        }
          res.status = 200;
          res.set_content(response, "text/plain");
    });
    
    server.Get("/last_block", [&bc](const auto &req, auto &res) {
            block b =bc.chain.back();
            cout<<"hello?"<<endl<<flush;
            string response;
            stringstream ss;
            json block_json;
            block_json["previous_hash"] = b.previous_hash;
            block_json["timestamp"] = b.timestamp;
            block_json["block_reward_receive"] = b.block_reward_receive;
            block_json["block_reward_address"] = b.block_reward_address;
            block_json["nonce"] = b.nonce;
            block_json["extended_nonce"] = b.nonce_2;
            block_json["hash"] = b.hash;
            block_json["height"] = b.height;
            json tx_array = json::array();
            for (const auto& trans : b.transactions) {
                json tx_json;
                tx_json["number"] = trans.number;
                tx_json["sender"] = trans.sender;
                tx_json["receiver"] = trans.receiver;
                tx_json["amount"] = trans.amount;
                tx_json["hash"] = trans.hash;
                tx_array.push_back(tx_json);
            }
            block_json["transactions"] = tx_array;
            ss<<block_json;
            ss>>response;
            cout<<response<<endl;
            res.status = 200;
          res.set_content(response, "text/plain");
    });
    
    server.Get("/pending_transactions", [&bc](const auto &req, auto &res) {

            string response;
            stringstream ss;
            json tx_array = json::array();
            for (const auto& trans : bc.pending_transactions) {
                json tx_json;
                tx_json["number"] = trans.number;
                tx_json["sender"] = trans.sender;
                tx_json["receiver"] = trans.receiver;
                tx_json["amount"] = trans.amount;
                tx_json["hash"] = trans.hash;
                tx_array.push_back(tx_json);
            }
            ss<<tx_array;
            ss>>response;
            cout<<response<<endl;
            res.status = 200;
          res.set_content(response, "text/plain");
    });
    
    thread server_thread([&server]() {
        server.listen("0.0.0.0", 8080);
    });
    
    int option=0;
    while(true){
      cout<<"\nblockchain CLI\n"
      <<"options:\n"
      <<"1: append transaction (requires private key)\n"
      <<"2: check balance\n"
      <<"3: create new wallet\n"
      <<"4: save chain\n"
      <<"5: mine latest block\n"
      <<"6: make new transaction as JSON\n"
      <<"7: make new transaction as JSON(ignore balance)\n"
      <<"8: get last block as JSON\n"
      <<"9: append a block as JSON\n"
      <<"10: verify chain\n"
      <<"11: override current transaction list with JSON\n";
      cin>>option;
      
      switch(option){
        case 1:{
          uint64_t private_key, amount, destination;
          cout<<"enter your private key(it won't be sent to the network) ";
          cin>>dec>>private_key;
          cout<<"enter amount ";
          cin>>dec>>amount;
          cout<<"enter recipient's public key ";
          cin>>dec>>destination;
          bc.add_transaction(bc.new_transaction(private_key,destination,amount,0));
          bc.trans_number++;
          break;
        }
        case 2:{
          uint64_t key;
          cout<<"enter address ";
          cin>>dec>>key;
          bool v;
          cout<<dec<<"coins = "<<bc.get_balance(key, v)<<endl;
          break;
        }
        case 3:{
          uint64_t priv_ =get_private();
          cout<<"generating private key\n";
          cout<<"your private key is "<<dec<<priv_<<endl<<"your public key is "<<get_public(priv_)<<endl;
          cout<<"use your private key to make transactions and your public key to receive transactions\nDO NOT LOSE your PRIVATE key\nLOSING IT MEANS THAT ALL OF YOUR COINS WILL BE LOST";
          break;
        }
        case 4:{
          string fil;
          cout<<"enter filename ";
          cin>>fil;
          bc.save_to_file(fil);
          break;
        }
        case 5:{
          uint64_t addr;
          cout<<"enter reward address ";
          cin>>addr;
          cout<<"mining block "<<dec<<bc.height;
          bc.mine_new(time(nullptr),addr);
          break;
        }
        case 10 :{
          if(bc.is_chain_valid()) cout<<"valid";
          break;
        }
        case 6:{
          uint64_t private_key, amount, destination;
          cout<<"enter your private key(it won't be sent to the network) ";
          cin>>dec>>private_key;
          cout<<"enter amount ";
          cin>>dec>>amount;
          cout<<"enter recipient's public key ";
          cin>>dec>>destination;
          transaction trans=bc.new_transaction(private_key,destination,amount,0);
            json tx_json;
            tx_json["number"] = trans.number;
            tx_json["sender"] = trans.sender;
            tx_json["receiver"] = trans.receiver;
            tx_json["amount"] = trans.amount;
            tx_json["hash"] = trans.hash;
          cout<<tx_json;
          break;
        }
        case 7:{
          uint64_t private_key, amount, destination;
          cout<<"enter your private key(it won't be sent to the network) ";
          cin>>dec>>private_key;
          cout<<"enter amount ";
          cin>>dec>>amount;
          cout<<"enter recipient's public key ";
          cin>>dec>>destination;
          transaction trans=bc.new_transaction(private_key,destination,amount,1);
            json tx_json;
            tx_json["number"] = trans.number;
            tx_json["sender"] = trans.sender;
            tx_json["receiver"] = trans.receiver;
            tx_json["amount"] = amount;
            tx_json["hash"] = trans.hash;
          cout<<tx_json;
          break;
        }
        case 8:{
            block b =bc.get_last_block();
            json block_json;
            block_json["previous_hash"] = b.previous_hash;
            block_json["timestamp"] = b.timestamp;
            block_json["block_reward_receive"] = b.block_reward_receive;
            block_json["block_reward_address"] = b.block_reward_address;
            block_json["nonce"] = b.nonce;
            block_json["hash"] = b.hash;
            block_json["height"] = b.height;
            json tx_array = json::array();
            for (const auto& trans : b.transactions) {
                json tx_json;
                tx_json["number"] = trans.number;
                tx_json["sender"] = trans.sender;
                tx_json["receiver"] = trans.receiver;
                tx_json["amount"] = trans.amount;
                tx_json["hash"] = trans.hash;
                tx_array.push_back(tx_json);
            }
            block_json["transactions"] = tx_array;
            cout<<block_json;
            break;
      }
      case 9:{
          block b;
          json block_json;
          cin>>block_json;
          b.previous_hash = block_json["previous_hash"];
          b.timestamp = block_json["timestamp"];
          b.block_reward_receive = block_json["block_reward_receive"];
          b.block_reward_address = block_json["block_reward_address"];
          b.hash = block_json["hash"];
          b.height=block_json["height"];
          b.nonce=block_json["nonce"];
          if (block_json.contains("transactions")&&block_json["transactions"].is_array()) {
              for (const auto& tx_json : block_json["transactions"]) {
                transaction trans;
                trans.number = tx_json["number"];
                trans.sender = tx_json["sender"];
                trans.receiver = tx_json["receiver"];
                trans.amount = tx_json["amount"];
                trans.hash = tx_json["hash"];
                b.transactions.push_back(trans);
              }
          }
          cout<<bc.send_block(b);
          break;
      }
      case 11:{
        json j;
        cin>>j;
        vector<transaction> pending;
        pending.reserve(256);
        for (const auto& tx_json : j) {
          transaction trans;
          trans.number = tx_json["number"];
          trans.sender = tx_json["sender"];
          trans.receiver = tx_json["receiver"];
          trans.amount = tx_json["amount"];
          trans.hash = tx_json["hash"];
          pending.push_back(trans);
        }
        bc.pending_transactions=pending;
        break;
      }
    }
    }
    return 0;
}
