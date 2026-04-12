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

using json = nlohmann::json;

using namespace std;


class block{
private:
  
  uint16_t size = 15;

public:
  vector<uint64_t>vec;
  uint64_t height;
  uint64_t previous_hash;
  uint64_t timestamp;
  vector<transaction> transactions;
  uint64_t block_reward_receive;
  PublicKey block_reward_address;
  uint64_t nonce=0;
  uint64_t nonce_2=0;
  uint64_t hash;
  
  vector<uint64_t> vectorize(){
    transaction tmp;
    vec.clear();
    vec.reserve(size);
    vec.push_back(nonce_2);
    vec.push_back(nonce);
    vec.push_back(height);
    vec.push_back(previous_hash);
    vec.push_back(timestamp);
    vector<uint64_t> v;
    v.reserve(transactions.size()*(1+5+5+8+1));
    for(uint16_t j = 0;j<transactions.size();j++){
      tmp=transactions.at(j);
      v.push_back(tmp.number);
      for(char i = 0;i<5;++i)
        v.push_back(tmp.sender.parts[i]);
      for(char i = 0;i<5;++i)
        v.push_back(tmp.receiver.parts[i]);
      v.push_back(tmp.amount);
      for(char i = 0;i<8;++i)
        v.push_back(tmp.signature[i]);
    }
    uint64_t a,b,c,d;
    tinyhash256(v,a,b,c,d);
    vec.push_back(a);
    vec.push_back(b);
    vec.push_back(c);
    vec.push_back(d);
    vec.push_back(block_reward_receive);
    for(char i = 0;i<5;++i)
      vec.push_back(block_reward_address.parts[i]);
    return vec;
  }
  void print_vectorization(const vector<uint64_t>& v) {
    cout << "Vectorizing " << v.size() << " values:\n";
    for (size_t i = 0; i < v.size() && i < 20; i++) {
        cout << "  [" << i << "] = " << v[i] << "\n";
      }
  }
  uint64_t calculate_hash(){
    vec=vectorize();
    return  tinyhash(vec);
  }
  bool isvalid(uint64_t difficulty){
    vec=vectorize();
    return (hash<difficulty)&&(calculate_hash()==hash);
  }
  void mine(uint64_t difficulty){
    bool enter=1;
    while(tinyhash(vec)>difficulty&&enter){
      enter=0;
      nonce = 0;
      vec=vectorize();
      while(vec.at(1)<0xffffffffffffffffULL){
        vec.at(1)++;
        if(tinyhash(vec)<=difficulty) goto exit;
        if(vec[1]&32768==0)cout<<vec[1]<<endl;
      }
      nonce_2++;
    }
    exit:
    nonce=vec.at(1);
    hash=calculate_hash();
    
    cout<<"block mined! hash = "<<hex<<hash<<"\nnonce = "<<nonce_2<<nonce;
    print_vectorization(vec);
  }
};






class blockchain{

public:
  uint64_t height=0;
  uint64_t trans_number;
  vector<block> chain;
  vector<transaction> pending_transactions;
  uint64_t difficulty=68719476736;
  uint64_t reward=524288;
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
    genesis.block_reward_receive=524288;
    PublicKey key = {17696393948516182787ULL,13902621260475358775ULL,15621036470522131904ULL,9159884703671929289ULL,242ULL};
    genesis.block_reward_address=key;
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
  uint64_t get_balance(PublicKey address, bool &valid){
    //this is tricky because it requires unwinding the blockchain
    
    block b;
    transaction tr;
    uint64_t balance=0;
    int i;
    for(i = 0; i<chain.size(); i++){
      b=chain.at(i);
      vector<transaction> t=b.transactions;
      if(b.block_reward_address.parts[0]==address.parts[0]&&
        b.block_reward_address.parts[1]==address.parts[1]&&
        b.block_reward_address.parts[2]==address.parts[2]&&
        b.block_reward_address.parts[3]==address.parts[3]&&
        b.block_reward_address.parts[4]==address.parts[4]
      )
        balance+=b.block_reward_receive;
      for(int j = 0; j<t.size(); j++){
        tr=t.at(j);
        if(tr.sender.parts[0]==address.parts[0]&&
        tr.sender.parts[1]==address.parts[1]&&
        tr.sender.parts[2]==address.parts[2]&&
        tr.sender.parts[3]==address.parts[3]&&
        tr.sender.parts[4]==address.parts[4]
          ){
          if(balance < tr.amount){
            cout<<"!negative balance!";
            valid=0;
            return 0;
          }
          balance-=tr.amount;
        }
        if(tr.receiver.parts[0]==address.parts[0]&&
        tr.receiver.parts[1]==address.parts[1]&&
        tr.receiver.parts[2]==address.parts[2]&&
        tr.receiver.parts[3]==address.parts[3]&&
        tr.receiver.parts[4]==address.parts[4]
          ) balance+=tr.amount;
      }
    }
    valid= 1;
    return balance;
  }
  void mine_new(uint64_t timestamp, PublicKey address){
    block b,l=get_last_block();
    height++;
    b.height=height;
  
    b.timestamp=timestamp;
    b.previous_hash=l.hash;
    b.transactions=pending_transactions;
    uint64_t total_transferred=0;
    for(auto i : b.transactions)
      total_transferred+=i.amount;
    pending_transactions.clear();
    pending_transactions.reserve(100);
    b.block_reward_receive=reward+(total_transferred/1024)+1;
    b.block_reward_address=address;
    b.mine(difficulty);
    chain.push_back(b);
    if(height%131072==0&&height>0) update_difficulty();
  }
  bool send_block(block b){
    if(b.isvalid(difficulty)){
      chain.push_back(b);
      return 0;
    }
    return 1;
  }
  transaction new_transaction(PrivateKey private_key, PublicKey receiver, uint64_t amount, bool ignore){
    transaction trans;
    trans.number=time(nullptr);
    get_public_from_private(private_key, trans.sender);
    trans.receiver=receiver;
    trans.amount=amount;
    bool valid;
    int64_t balance = get_balance(trans.sender, valid);
    if (!valid) trans.amount=0;
    if (balance<trans.amount&&!ignore){
      cout<<"you do not have enough money "<<balance<<endl;
      trans.amount=0;
    }
    sign(trans, private_key);
    return trans;
  }
  
  void update_difficulty(){
    if(height>=131072){ reward=458752; difficulty=4294967296;}
    if(height>=262144){ reward=393216; difficulty=536870912;}
    if(height>=393216){ reward=327680; difficulty=134217728;}
    if(height>=524288){ reward=262144; difficulty=33554432;}
    if(height>=655360){ reward=229376; difficulty=8388608;}
    if(height>=786432){ reward=196608; difficulty=2097152;}
    if(height>=917504){ reward=163840; difficulty=524288;}
    if(height>=1048576){ reward=131072; difficulty=262144;}
    if(height>=1179648){ reward=114688; difficulty=131072;}
    if(height>=1310720){ reward=98304; difficulty=32768;}
  }

  
    bool save_to_file(const string& filename) {
        json j;
        
        json chain_array = json::array();
        for (const auto& b : chain) {
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
            
            chain_array.push_back(block_json);
        }
        
        j["chain"] = chain_array;
        j["difficulty"] = difficulty;
        j["reward"] = reward;
        j["height"]=height;
        j["miners"]=miners;
        json tx_array = json::array();
        for (const auto& trans : pending_transactions) {
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
        j["pending_transactions"] = tx_array;
        
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
          b.block_reward_address.parts[0]=block_json["r0"];
          b.block_reward_address.parts[1]=block_json["r1"];
          b.block_reward_address.parts[2]=block_json["r2"];
          b.block_reward_address.parts[3]=block_json["r3"];
          b.block_reward_address.parts[4]=block_json["r4"];
          b.hash = block_json["hash"];
          b.height=block_json["height"];
          b.nonce=block_json["nonce"];
          b.nonce_2=block_json["extended_nonce"];
          if (block_json.contains("transactions")&&block_json["transactions"].is_array()) {
              for (const auto& tx_json : block_json["transactions"]) {
                transaction trans;
                trans.number = tx_json["number"];
                trans.sender.parts[0] = tx_json["s0"];
                trans.sender.parts[1] = tx_json["s0"];
                trans.sender.parts[2] = tx_json["s0"];
                trans.sender.parts[3] = tx_json["s0"];
                trans.sender.parts[4] = tx_json["s0"];
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
                trans.sender.parts[0] = tx_json["s0"];
                trans.sender.parts[1] = tx_json["s0"];
                trans.sender.parts[2] = tx_json["s0"];
                trans.sender.parts[3] = tx_json["s0"];
                trans.sender.parts[4] = tx_json["s0"];
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
        if (hash != tinyhash(current.vec)) {
          cout<<"block " << i << " hash mismatch! Stored: " <<dec<< hash<<" calculated: "<< current.calculate_hash()<< endl;
          return 0;
        }
        for(auto i : current.transactions){
          if(!verify(i)){
            cout<<"A TRANSACTION IS NOT SIGNED!!";
            return 0;
          }
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
          PrivateKey pri;
          PublicKey pub;
          generate_keypair(pri, pub);
          stringstream ss;
          ss<<"private"<<"key=\n"
          <<"0:"<<hex<<pri.parts[0]<<endl
          <<"1:"<<hex<<pri.parts[1]<<endl
          <<"2:"<<hex<<pri.parts[2]<<endl
          <<"3:"<<hex<<pri.parts[3]<<endl
          <<"\npublic"<<"key=\n"
          <<"0:"<<hex<<pub.parts[0]<<endl
          <<"1:"<<hex<<pub.parts[1]<<endl
          <<"2:"<<hex<<pub.parts[2]<<endl
          <<"3:"<<hex<<pub.parts[3]<<endl
          <<"4:"<<hex<<pub.parts[4]<<endl;
          string response=ss.str();
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
        string q = req.get_param_value("q");
        if (q.length() < 80) {
            res.status = 400;
            res.set_content("Invalid public key length", "text/plain");
            return;
        }
        PublicKey key;
        memset(key.parts, 0, sizeof(key.parts));
        for (int i = 0; i < 5; i++) {
            string chunk = q.substr(i * 16, 16);
            key.parts[i] = stoull(chunk, nullptr, 16);
        }
        bool valid;
        uint64_t balance = bc.get_balance(key, valid);
        if (!valid) {
            res.status = 400;
            res.set_content("Invalid balance (negative or corrupted chain)", "text/plain");
            return;
        }
        string response = "balance = " + to_string(balance);
        res.status = 200;
        res.set_content(response, "text/plain");
        
});
    server.Post("/new_transaction", [&bc](const auto &req, auto &res) {
          json tx_json = json::parse(req.body);
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
          bool valid=0;
          string response;
          verify(trans);
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
          json block_json=json::parse(req.body);
          block b;
          cout<<"hello";
          b.previous_hash = block_json["previous_hash"];
          b.timestamp = block_json["timestamp"];
          b.block_reward_receive = block_json["block_reward_receive"];
          b.block_reward_address.parts[0]=block_json["r0"];
          b.block_reward_address.parts[1]=block_json["r1"];
          b.block_reward_address.parts[2]=block_json["r2"];
          b.block_reward_address.parts[3]=block_json["r3"];
          b.block_reward_address.parts[4]=block_json["r4"];
          b.hash = block_json["hash"];
          b.height=block_json["height"];
          b.nonce=block_json["nonce"];
          b.nonce_2=block_json["extended_nonce"];
          if (block_json.contains("transactions")&&block_json["transactions"].is_array()) {
              for (const auto& tx_json : block_json["transactions"]) {
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
                b.transactions.push_back(trans);
              }
          }
          string response;
          b.print_vectorization(b.vectorize());
          cout<<"submissions: "<<submissions<<endl;
          cout<<dec<<tinyhash(b.vectorize())<<flush;
          bool valid=tinyhash(b.vectorize())==b.hash;
          for(auto i : b.transactions){
              if (!verify(i)){
                valid = 0;
                cout<<"invalid transaction";
                break;
              }
          }
          if(!valid){
             response = "INVALID BLOCK, rejected";
          }
          else{
            response = "VALID, waiting for verification";
            buffer.push_back(b);
            uint64_t tmp_time = b.timestamp, tmp_nonce=b.nonce, tmp_exnonce=b.nonce_2;
            PublicKey tmp_key = b.block_reward_address;
            b.timestamp=0;
            b.nonce=0;
            b.nonce_2=0;
            b.block_reward_address={0,0,0,0,0};
            vector<uint64_t> vc;
            vc=b.vectorize();
            hash_buffer.push_back(tinyhash(vc));
            b.timestamp=tmp_time;
            b.nonce=tmp_nonce;
            b.nonce_2=tmp_exnonce;
            b.block_reward_address=tmp_key;
            submissions++;
            if (submissions>=bc.miners){
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
                    if (count > maxcount || (count == maxcount && hash_buffer[i] > res)) {
                       maxcount = count;
                       res = hash_buffer[i];
                    }
               }
              //res contains the most common
              //linear search
              //give the reward to who finished first
              uint64_t prev_time=0, min_time = 200000000000000;
              uint64_t majority_ix=0, min_ix=0;
              bool valid=1;
              for(int i = 0; i<n;i++){
                if(prev_time==buffer.at(i).timestamp){
                  cout<<"same timestamp"<<endl;
                  valid=0;
                }
                if(hash_buffer.at(i)==res){
                  prev_time=buffer.at(i).timestamp;
                  majority_ix=i;
                  if(min_time>buffer.at(i).timestamp){
                    min_time=buffer.at(i).timestamp;
                    min_ix=i;
                  }
                }
              }
              if(!valid){
                response="VERIFICATION FAILED";
                submissions--;
              }
              else{
                response="PASSED, appending to blockchain";
                //first valid block
                bc.chain.push_back(buffer.at(min_ix));
                bc.save_to_file("chain.json");
              }
              buffer.clear();
              hash_buffer.clear();
              bc.pending_transactions.clear();
              submissions=0;
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
            ss<<tx_array;
            ss>>response;
            cout<<response<<endl;
            res.status = 200;
          res.set_content(response, "text/plain");
    });
    
    thread server_thread([&server]() {
        server.listen("0.0.0.0", 8081);
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
          uint64_t amount;
          PrivateKey private_key;
          PublicKey destination;
          cout<<"enter your private key(it won't be sent to the network)";
          for(int i = 0; i<4; i++){
            cout<<endl<<i<<": ";
            cin>>hex>>private_key.parts[i];
          }
          cout<<"enter amount ";
          cin>>dec>>amount;
          cout<<"enter recipient's public key ";
          for(int i = 0; i<5; i++){
            cout<<endl<<i<<": ";
            cin>>hex>>destination.parts[i];
          }
          bc.add_transaction(bc.new_transaction(private_key,destination,amount,0));
          bc.trans_number++;
          break;
        }
        case 2:{
          PublicKey key;
          cout<<"enter address ";
          for(int i = 0; i<5; i++){
            cout<<endl<<i<<": ";
            cin>>hex>>key.parts[i];
          }
          bool v;
          cout<<dec<<"coins = "<<bc.get_balance(key, v)<<endl;
          break;
        }
        case 3:{
          PrivateKey priv_;
          PublicKey pub;
          generate_keypair(priv_, pub);
          cout<<"Private key(write this down on a piece of paper)\n";
          for(int i = 0; i<4; i++){
            cout<<i<<"    "<<hex<<priv_.parts[i]<<endl;
          }
          cout<<"Public key\n";
          for(int i = 0; i<5; i++){
            cout<<i<<"    "<<hex<<pub.parts[i]<<endl;
          }
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
          PublicKey addr;
          cout<<"enter reward address ";
          for(int i = 0; i<5; i++){
            cout<<endl<<i<<": ";
            cin>>hex>>addr.parts[i];
          }
          cout<<"mining block "<<dec<<bc.height;
          bc.mine_new(time(nullptr),addr);
          break;
        }
        case 10 :{
          if(bc.is_chain_valid()) cout<<"valid";
          break;
        }
        case 6:{
          uint64_t amount;
          PrivateKey private_key;
          PublicKey destination;
          cout<<"enter your private key(it won't be sent to the network)";
          for(int i = 0; i<4; i++){
            cout<<endl<<i<<": ";
            cin>>hex>>private_key.parts[i];
          }
          cout<<"enter amount ";
          cin>>dec>>amount;
          cout<<"enter recipient's public key ";
          for(int i = 0; i<5; i++){
            cout<<endl<<i<<": ";
            cin>>hex>>destination.parts[i];
          }
          transaction trans = bc.new_transaction(private_key,destination,amount,0);
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
          cout<<tx_json;
          break;
        }
        case 7:{
          uint64_t amount;
          PrivateKey private_key;
          PublicKey destination;
          cout<<"enter your private key(it won't be sent to the network)";
          for(int i = 0; i<4; i++){
            cout<<endl<<i<<": ";
            cin>>hex>>private_key.parts[i];
          }
          cout<<"enter amount ";
          cin>>dec>>amount;
          cout<<"enter recipient's public key ";
          for(int i = 0; i<5; i++){
            cout<<endl<<i<<": ";
            cin>>hex>>destination.parts[i];
          }
          transaction trans = bc.new_transaction(private_key,destination,amount,1);
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
          cout<<tx_json;
          break;
        }
        case 8:{
            block b =bc.get_last_block();
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
          b.block_reward_address.parts[0]=block_json["r0"];
          b.block_reward_address.parts[1]=block_json["r1"];
          b.block_reward_address.parts[2]=block_json["r2"];
          b.block_reward_address.parts[3]=block_json["r3"];
          b.block_reward_address.parts[4]=block_json["r4"];
          b.hash = block_json["hash"];
          b.height=block_json["height"];
          b.nonce=block_json["nonce"];
          b.nonce_2=block_json["extended_nonce"];
          if (block_json.contains("transactions")&&block_json["transactions"].is_array()) {
              for (const auto& tx_json : block_json["transactions"]) {
                transaction trans;
                trans.number = tx_json["number"];
                trans.sender.parts[0] = tx_json["s0"];
                trans.sender.parts[1] = tx_json["s0"];
                trans.sender.parts[2] = tx_json["s0"];
                trans.sender.parts[3] = tx_json["s0"];
                trans.sender.parts[4] = tx_json["s0"];
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
        pending.reserve(100);
        for (const auto& tx_json : j) {
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
                pending.push_back(trans);
        }
        bc.pending_transactions=pending;
        break;
      }
    }
    }
    return 0;
}#include <iostream>
#include <vector>
#include <cstdint>
#include <iomanip>
#include <random>
#include "json.hpp"
#include "httplib.h"
#include <fstream>
#include <sstream>

#include "cryptography.cpp"

using json = nlohmann::json;

using namespace std;


class block{
private:
  
  uint16_t size = 15;

public:
  vector<uint64_t>vec;
  uint64_t height;
  uint64_t previous_hash;
  uint64_t timestamp;
  vector<transaction> transactions;
  uint64_t block_reward_receive;
  PublicKey block_reward_address;
  uint64_t nonce=0;
  uint64_t nonce_2=0;
  uint64_t hash;
  
  vector<uint64_t> vectorize(){
    transaction tmp;
    vec.clear();
    vec.reserve(size);
    vec.push_back(nonce_2);
    vec.push_back(nonce);
    vec.push_back(height);
    vec.push_back(previous_hash);
    vec.push_back(timestamp);
    vector<uint64_t> v;
    v.reserve(transactions.size()*(1+5+5+8+1));
    for(uint16_t j = 0;j<transactions.size();j++){
      tmp=transactions.at(j);
      v.push_back(tmp.number);
      for(char i = 0;i<5;++i)
        v.push_back(tmp.sender.parts[i]);
      for(char i = 0;i<5;++i)
        v.push_back(tmp.receiver.parts[i]);
      v.push_back(tmp.amount);
      for(char i = 0;i<8;++i)
        v.push_back(tmp.signature[i]);
    }
    uint64_t a,b,c,d;
    tinyhash256(v,a,b,c,d);
    vec.push_back(a);
    vec.push_back(b);
    vec.push_back(c);
    vec.push_back(d);
    vec.push_back(block_reward_receive);
    for(char i = 0;i<5;++i)
      vec.push_back(block_reward_address.parts[i]);
    return vec;
  }
  void print_vectorization(const vector<uint64_t>& v) {
    cout << "Vectorizing " << v.size() << " values:\n";
    for (size_t i = 0; i < v.size() && i < 20; i++) {
        cout << "  [" << i << "] = " << v[i] << "\n";
      }
  }
  uint64_t calculate_hash(){
    vec=vectorize();
    return  tinyhash(vec);
  }
  bool isvalid(uint64_t difficulty){
    vec=vectorize();
    return (hash<difficulty)&&(calculate_hash()==hash);
  }
  void mine(uint64_t difficulty){
    bool enter=1;
    while(tinyhash(vec)>difficulty&&enter){
      enter=0;
      nonce = 0;
      vec=vectorize();
      while(vec.at(1)<0xffffffffffffffffULL){
        vec.at(1)++;
        if(tinyhash(vec)<=difficulty) goto exit;
        if(vec[1]&32768==0)cout<<vec[1]<<endl;
      }
      nonce_2++;
    }
    exit:
    nonce=vec.at(1);
    hash=calculate_hash();
    
    cout<<"block mined! hash = "<<hex<<hash<<"\nnonce = "<<nonce_2<<nonce;
    print_vectorization(vec);
  }
};






class blockchain{

public:
  uint64_t height=0;
  uint64_t trans_number;
  vector<block> chain;
  vector<transaction> pending_transactions;
  uint64_t difficulty=1099511627776;
  uint64_t reward=524288;
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
    genesis.block_reward_receive=524288;
    PublicKey key = {17696393948516182787,13902621260475358775,15621036470522131904,9159884703671929289,242};
    genesis.block_reward_address=key;
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
  uint64_t get_balance(PublicKey address, bool &valid){
    //this is tricky because it requires unwinding the blockchain
    
    block b;
    transaction tr;
    uint64_t balance=0;
    int i;
    for(i = 0; i<chain.size(); i++){
      b=chain.at(i);
      vector<transaction> t=b.transactions;
      if(b.block_reward_address.parts[0]==address.parts[0]&&
        b.block_reward_address.parts[1]==address.parts[1]&&
        b.block_reward_address.parts[2]==address.parts[2]&&
        b.block_reward_address.parts[3]==address.parts[3]&&
        b.block_reward_address.parts[4]==address.parts[4]
      )
        balance+=b.block_reward_receive;
      for(int j = 0; j<t.size(); j++){
        tr=t.at(j);
        if(tr.sender.parts[0]==address.parts[0]&&
        tr.sender.parts[1]==address.parts[1]&&
        tr.sender.parts[2]==address.parts[2]&&
        tr.sender.parts[3]==address.parts[3]&&
        tr.sender.parts[4]==address.parts[4]
          ){
          if(balance < tr.amount){
            cout<<"!negative balance!";
            valid=0;
            return 0;
          }
          balance-=tr.amount;
        }
        if(tr.receiver.parts[0]==address.parts[0]&&
        tr.receiver.parts[1]==address.parts[1]&&
        tr.receiver.parts[2]==address.parts[2]&&
        tr.receiver.parts[3]==address.parts[3]&&
        tr.receiver.parts[4]==address.parts[4]
          ) balance+=tr.amount;
      }
    }
    valid= 1;
    return balance;
  }
  void mine_new(uint64_t timestamp, PublicKey address){
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
    if(height%131072==0&&height>0) update_difficulty();
  }
  bool send_block(block b){
    if(b.isvalid(difficulty)){
      chain.push_back(b);
      return 0;
    }
    return 1;
  }
  transaction new_transaction(PrivateKey private_key, PublicKey receiver, uint64_t amount, bool ignore){
    transaction trans;
    trans.number=time(nullptr);
    get_public_from_private(private_key, trans.sender);
    trans.receiver=receiver;
    trans.amount=amount;
    bool valid;
    int64_t balance = get_balance(trans.sender, valid);
    if (!valid) trans.amount=0;
    if (balance<trans.amount&&!ignore){
      cout<<"you do not have enough money "<<balance<<endl;
      trans.amount=0;
    }
    sign(trans, private_key);
    return trans;
  }
  
  void update_difficulty(){
    if(height>=131072){ reward=458752; difficulty=68719476736;}
    if(height>=262144){ reward=393216; difficulty=4294967296;}
    if(height>=393216){ reward=327680; difficulty=536870912;}
    if(height>=524288){ reward=262144; difficulty=134217728;}
    if(height>=655360){ reward=229376; difficulty=33554432;}
    if(height>=786432){ reward=196608; difficulty=8388608;}
    if(height>=917504){ reward=163840; difficulty=2097152;}
    if(height>=1048576){ reward=131072; difficulty=524288;}
    if(height>=1179648){ reward=114688; difficulty=262144;}
    if(height>=1310720){ reward=98304; difficulty=131072;}
  }

  
    bool save_to_file(const string& filename) {
        json j;
        
        json chain_array = json::array();
        for (const auto& b : chain) {
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
            
            chain_array.push_back(block_json);
        }
        
        j["chain"] = chain_array;
        j["difficulty"] = difficulty;
        j["reward"] = reward;
        j["height"]=height;
        j["miners"]=miners;
        json tx_array = json::array();
        for (const auto& trans : pending_transactions) {
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
        j["pending_transactions"] = tx_array;
        
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
          b.block_reward_address.parts[0]=block_json["r0"];
          b.block_reward_address.parts[1]=block_json["r1"];
          b.block_reward_address.parts[2]=block_json["r2"];
          b.block_reward_address.parts[3]=block_json["r3"];
          b.block_reward_address.parts[4]=block_json["r4"];
          b.hash = block_json["hash"];
          b.height=block_json["height"];
          b.nonce=block_json["nonce"];
          b.nonce_2=block_json["extended_nonce"];
          if (block_json.contains("transactions")&&block_json["transactions"].is_array()) {
              for (const auto& tx_json : block_json["transactions"]) {
                transaction trans;
                trans.number = tx_json["number"];
                trans.sender.parts[0] = tx_json["s0"];
                trans.sender.parts[1] = tx_json["s0"];
                trans.sender.parts[2] = tx_json["s0"];
                trans.sender.parts[3] = tx_json["s0"];
                trans.sender.parts[4] = tx_json["s0"];
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
                trans.sender.parts[0] = tx_json["s0"];
                trans.sender.parts[1] = tx_json["s0"];
                trans.sender.parts[2] = tx_json["s0"];
                trans.sender.parts[3] = tx_json["s0"];
                trans.sender.parts[4] = tx_json["s0"];
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
        if (hash != tinyhash(current.vec)) {
          cout<<"block " << i << " hash mismatch! Stored: " <<dec<< hash<<" calculated: "<< current.calculate_hash()<< endl;
          return 0;
        }
        for(auto i : current.transactions){
          if(!verify(i)){
            cout<<"A TRANSACTION IS NOT SIGNED!!";
            return 0;
          }
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
          PrivateKey pri;
          PublicKey pub;
          generate_keypair(pri, pub);
          stringstream ss;
          ss<<"private"<<"key=\n"
          <<"0:"<<hex<<pri.parts[0]<<endl
          <<"1:"<<hex<<pri.parts[1]<<endl
          <<"2:"<<hex<<pri.parts[2]<<endl
          <<"3:"<<hex<<pri.parts[3]<<endl
          <<"\npublic"<<"key=\n"
          <<"0:"<<hex<<pub.parts[0]<<endl
          <<"1:"<<hex<<pub.parts[1]<<endl
          <<"2:"<<hex<<pub.parts[2]<<endl
          <<"3:"<<hex<<pub.parts[3]<<endl
          <<"4:"<<hex<<pub.parts[4]<<endl;
          string response=ss.str();
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
        string q = req.get_param_value("q");
        if (q.length() < 80) {
            res.status = 400;
            res.set_content("Invalid public key length", "text/plain");
            return;
        }
        PublicKey key;
        memset(key.parts, 0, sizeof(key.parts));
        for (int i = 0; i < 5; i++) {
            string chunk = q.substr(i * 16, 16);
            key.parts[i] = stoull(chunk, nullptr, 16);
        }
        bool valid;
        uint64_t balance = bc.get_balance(key, valid);
        if (!valid) {
            res.status = 400;
            res.set_content("Invalid balance (negative or corrupted chain)", "text/plain");
            return;
        }
        string response = "balance = " + to_string(balance);
        res.status = 200;
        res.set_content(response, "text/plain");
        
});
    server.Post("/new_transaction", [&bc](const auto &req, auto &res) {
          json tx_json = json::parse(req.body);
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
          bool valid=0;
          string response;
          verify(trans);
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
          json block_json=json::parse(req.body);
          block b;
          cout<<"hello";
          b.previous_hash = block_json["previous_hash"];
          b.timestamp = block_json["timestamp"];
          b.block_reward_receive = block_json["block_reward_receive"];
          b.block_reward_address.parts[0]=block_json["r0"];
          b.block_reward_address.parts[1]=block_json["r1"];
          b.block_reward_address.parts[2]=block_json["r2"];
          b.block_reward_address.parts[3]=block_json["r3"];
          b.block_reward_address.parts[4]=block_json["r4"];
          b.hash = block_json["hash"];
          b.height=block_json["height"];
          b.nonce=block_json["nonce"];
          b.nonce_2=block_json["extended_nonce"];
          if (block_json.contains("transactions")&&block_json["transactions"].is_array()) {
              for (const auto& tx_json : block_json["transactions"]) {
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
                b.transactions.push_back(trans);
              }
          }
          string response;
          b.print_vectorization(b.vectorize());
          cout<<"submissions: "<<submissions<<endl;
          cout<<dec<<tinyhash(b.vectorize())<<flush;
          bool valid=tinyhash(b.vectorize())==b.hash;
          for(auto i : b.transactions){
              if (!verify(i)){
                valid = 0;
                cout<<"invalid transaction";
                break;
              }
          }
          if(!valid){
             response = "INVALID BLOCK, rejected";
          }
          else{
            response = "VALID, waiting for verification";
            buffer.push_back(b);
            uint64_t tmp_time = b.timestamp, tmp_nonce=b.nonce, tmp_exnonce=b.nonce_2;
            PublicKey tmp_key = b.block_reward_address;
            b.timestamp=0;
            b.nonce=0;
            b.nonce_2=0;
            b.block_reward_address={0,0,0,0,0};
            vector<uint64_t> vc;
            vc=b.vectorize();
            hash_buffer.push_back(tinyhash(vc));
            b.timestamp=tmp_time;
            b.nonce=tmp_nonce;
            b.nonce_2=tmp_exnonce;
            b.block_reward_address=tmp_key;
            submissions++;
            if (submissions>=bc.miners){
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
                    if (count > maxcount || (count == maxcount && hash_buffer[i] > res)) {
                       maxcount = count;
                       res = hash_buffer[i];
                    }
               }
              //res contains the most common
              //linear search
              //give the reward to who finished first
              uint64_t prev_time=0, min_time = 200000000000000;
              uint64_t majority_ix=0, min_ix=0;
              bool valid=1;
              for(int i = 0; i<n;i++){
                if(prev_time==buffer.at(i).timestamp){
                  cout<<"same timestamp"<<endl;
                  valid=0;
                }
                if(hash_buffer.at(i)==res){
                  prev_time=buffer.at(i).timestamp;
                  majority_ix=i;
                  if(min_time>buffer.at(i).timestamp){
                    min_time=buffer.at(i).timestamp;
                    min_ix=i;
                  }
                }
              }
              if(!valid){
                response="VERIFICATION FAILED";
                submissions--;
              }
              else{
                response="PASSED, appending to blockchain";
                //first valid block
                bc.chain.push_back(buffer.at(min_ix));
                bc.save_to_file("chain.json");
              }
              buffer.clear();
              hash_buffer.clear();
              bc.pending_transactions.clear();
              submissions=0;
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
            ss<<tx_array;
            ss>>response;
            cout<<response<<endl;
            res.status = 200;
          res.set_content(response, "text/plain");
    });
    
    thread server_thread([&server]() {
        server.listen("0.0.0.0", 8081);
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
          uint64_t amount;
          PrivateKey private_key;
          PublicKey destination;
          cout<<"enter your private key(it won't be sent to the network)";
          for(int i = 0; i<4; i++){
            cout<<endl<<i<<": ";
            cin>>hex>>private_key.parts[i];
          }
          cout<<"enter amount ";
          cin>>dec>>amount;
          cout<<"enter recipient's public key ";
          for(int i = 0; i<5; i++){
            cout<<endl<<i<<": ";
            cin>>hex>>destination.parts[i];
          }
          bc.add_transaction(bc.new_transaction(private_key,destination,amount,0));
          bc.trans_number++;
          break;
        }
        case 2:{
          PublicKey key;
          cout<<"enter address ";
          for(int i = 0; i<5; i++){
            cout<<endl<<i<<": ";
            cin>>hex>>key.parts[i];
          }
          bool v;
          cout<<dec<<"coins = "<<bc.get_balance(key, v)<<endl;
          break;
        }
        case 3:{
          PrivateKey priv_;
          PublicKey pub;
          generate_keypair(priv_, pub);
          cout<<"Private key(write this down on a piece of paper)\n";
          for(int i = 0; i<4; i++){
            cout<<i<<"    "<<hex<<priv_.parts[i]<<endl;
          }
          cout<<"Public key\n";
          for(int i = 0; i<5; i++){
            cout<<i<<"    "<<hex<<pub.parts[i]<<endl;
          }
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
          PublicKey addr;
          cout<<"enter reward address ";
          for(int i = 0; i<5; i++){
            cout<<endl<<i<<": ";
            cin>>hex>>addr.parts[i];
          }
          cout<<"mining block "<<dec<<bc.height;
          bc.mine_new(time(nullptr),addr);
          break;
        }
        case 10 :{
          if(bc.is_chain_valid()) cout<<"valid";
          break;
        }
        case 6:{
          uint64_t amount;
          PrivateKey private_key;
          PublicKey destination;
          cout<<"enter your private key(it won't be sent to the network)";
          for(int i = 0; i<4; i++){
            cout<<endl<<i<<": ";
            cin>>hex>>private_key.parts[i];
          }
          cout<<"enter amount ";
          cin>>dec>>amount;
          cout<<"enter recipient's public key ";
          for(int i = 0; i<5; i++){
            cout<<endl<<i<<": ";
            cin>>hex>>destination.parts[i];
          }
          transaction trans = bc.new_transaction(private_key,destination,amount,0);
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
          cout<<tx_json;
          break;
        }
        case 7:{
          uint64_t amount;
          PrivateKey private_key;
          PublicKey destination;
          cout<<"enter your private key(it won't be sent to the network)";
          for(int i = 0; i<4; i++){
            cout<<endl<<i<<": ";
            cin>>hex>>private_key.parts[i];
          }
          cout<<"enter amount ";
          cin>>dec>>amount;
          cout<<"enter recipient's public key ";
          for(int i = 0; i<5; i++){
            cout<<endl<<i<<": ";
            cin>>hex>>destination.parts[i];
          }
          transaction trans = bc.new_transaction(private_key,destination,amount,1);
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
          cout<<tx_json;
          break;
        }
        case 8:{
            block b =bc.get_last_block();
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
          b.block_reward_address.parts[0]=block_json["r0"];
          b.block_reward_address.parts[1]=block_json["r1"];
          b.block_reward_address.parts[2]=block_json["r2"];
          b.block_reward_address.parts[3]=block_json["r3"];
          b.block_reward_address.parts[4]=block_json["r4"];
          b.hash = block_json["hash"];
          b.height=block_json["height"];
          b.nonce=block_json["nonce"];
          b.nonce_2=block_json["extended_nonce"];
          if (block_json.contains("transactions")&&block_json["transactions"].is_array()) {
              for (const auto& tx_json : block_json["transactions"]) {
                transaction trans;
                trans.number = tx_json["number"];
                trans.sender.parts[0] = tx_json["s0"];
                trans.sender.parts[1] = tx_json["s0"];
                trans.sender.parts[2] = tx_json["s0"];
                trans.sender.parts[3] = tx_json["s0"];
                trans.sender.parts[4] = tx_json["s0"];
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
        pending.reserve(100);
        for (const auto& tx_json : j) {
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
                pending.push_back(trans);
        }
        bc.pending_transactions=pending;
        break;
      }
    }
    }
    return 0;
}
