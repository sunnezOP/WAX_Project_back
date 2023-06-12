#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/system.hpp>

using namespace eosio;
using namespace std;

CONTRACT industrialcraft : public contract {
    // the name of the tokens contract account
    static constexpr eosio::name TOKEN_CONTRACT = name("industrialtt");
    static constexpr eosio::name collection_name = name("industrialct");

    //tokens
    static constexpr eosio::symbol ICS_SYMBOL = symbol("ICS", 4);
    static constexpr eosio::symbol ICI_SYMBOL = symbol("ICI", 4);
    static constexpr eosio::symbol ICG_SYMBOL = symbol("ICG", 4);



    //tools
    //Pickaxe
    static constexpr int32_t Common_Pickaxe = 606635;
    static constexpr int32_t Rare_Pickaxe = 606636;
    static constexpr int32_t Legendary_Pickaxe = 606638;
    static constexpr int32_t Industrial_Pickaxe = 606639;

    //Drill
    static constexpr int32_t Common_Drill = 607305;
    static constexpr int32_t Rare_Drill = 607306;
    static constexpr int32_t Legendary_Drill = 607307;
    static constexpr int32_t Industrial_Drill = 607308;

    //MineMachine
    static constexpr int32_t Common_Mine_Machine = 607309;
    static constexpr int32_t Rare_Mine_Machine = 607310;
    static constexpr int32_t Legendary_Mine_Machine = 607311;
    static constexpr int32_t Industrial_Mine_Machine = 607312;

    static constexpr uint8_t pickaxe = 1;
    static constexpr uint8_t drill = 2;
    static constexpr uint8_t mine_machine = 3;

    //static const std::vector<int32_t> temp;// = { Common_Pickaxe, Rare_Pickaxe, Legendary_Pickaxe, Industrial_Pickaxe };

    //const map<int32_t, int8_t> uses_map = {{Common_Pickaxe, 10}, {Rare_Pickaxe, 15}, {Legendary_Pickaxe, 20}, {Industrial_Pickaxe, 40}};
    

  public:
    using contract::contract;
    
    [[eosio::on_notify("atomicassets::transfer")]] void stake (
            name from,
            name to,
            vector <uint64_t> asset_ids,
            string memo
  );

    [[eosio::on_notify("industrialtt::transfer")]] void tokentransfer (
            name from,
            name to,
            asset quantity,
            string memo
    );

    ACTION regnewuser(const name &user);

    ACTION farm(const name &user,const uint64_t &assets_id);
    ACTION unfarm(const name &user,const uint64_t &assets_id);
//    ACTION syncfarm(const name &user,const uint64_t &asset_id);

    ACTION fullrepair(const name &user,const uint64_t &asset_id);

    ACTION withdrawtool(const name &user,const std::vector <uint64_t> &assets_id);
    ACTION withdraw(const name &user, asset quantity);

    ACTION delstaket(name user);

    ACTION delreguserst();
    ACTION deluserstate(name user);

    ACTION settoolconf(const int32_t &template_id,
                       const asset &ics_repair, const asset &ici_repair, const asset &icg_repair,
                       const asset &mine,
                       const asset &ics_craft, const asset &ici_craft, const asset &icg_craft,
                       const uint8_t &uses, const uint32_t &duration);
    /*ACTION settoolconf(const int32_t &template_id,
                       const asset &ics_repair, const asset &ici_repair, const asset &icg_repair,
                       const asset &mine,
                       const asset &ics_craft, const asset &ici_craft, const asset &icg_craft,
                       const uint8_t &uses);*/
    ACTION ralltoolconf();
    ACTION rtoolconfig(const int32_t &template_id);

    ACTION setstuckconf(int32_t template_id, uint8_t initial, uint8_t increase, uint8_t time_hours);
    ACTION rallstuckcon();
    ACTION rstuckconfig(const int32_t &template_id);

    ACTION settokenconf(uint8_t id, string token, uint8_t fee);
    ACTION ralltokencon();
    ACTION rtokenconfig(uint8_t id);

    ACTION craft(const name &schema_name, const int32_t &template_id, const name &new_asset_owner);
    ACTION randtest();

  private:
    TABLE account_entity {
        name account;
        uint16_t pickaxe;
        uint16_t drill;
        uint16_t mine_machine;
        asset ics;
        asset ici;
        asset icg;

        auto primary_key() const {
            return account.value;
        }
    };
    typedef multi_index<name("accounts"), account_entity> accounts_table;

    TABLE userstate_t{
            //uint64_t ID;
            uint8_t type;
            uint64_t assets_id;
            //bool available;
            time_point_sec time;

            auto primary_key() const {
                return assets_id;
            }
    };
    typedef multi_index<name("userstate"), userstate_t> userstate;

    TABLE toolconfig {
            int32_t template_id;
            asset ics_repair;
            asset ici_repair;
            asset icg_repair;
            asset mine;
            asset ics_craft;
            asset ici_craft;
            asset icg_craft;
            uint8_t uses;
            uint32_t duration;

            auto primary_key() const {
                return template_id;
            }
    };
    typedef multi_index<name("toolconf"), toolconfig> toolconfig_t;

    TABLE stuckconfig {
        int32_t template_id;
        uint8_t initial;
        uint8_t increase;
        uint8_t time_hours;

        auto primary_key() const {
            return template_id;
        }
    };
    typedef multi_index<name("stuckconf"), stuckconfig> stuckconfig_t;

    TABLE stucktool {
            uint64_t asset_id;
            int32_t template_id;
            name account;
            uint8_t chance;
            uint8_t initial;
            uint8_t increase;
            uint8_t time_hours;
            time_point_sec time;
            bool availability;

            auto primary_key() const {
                return asset_id;
            }
    };
    typedef multi_index<name("stucktools"), stucktool> stucktool_t;

    TABLE tokenconfig {

        uint8_t id;
        string token;
        uint8_t fee;

        auto primary_key() const {
            return id;
        }
    };
    typedef multi_index<name("tokenconf"), tokenconfig> tokenconfig_t;
    
    TABLE stake_t {

        uint64_t asset_id;
        name account;
        int32_t template_id;
        uint8_t uses;
        bool availability;

        uint64_t primary_key() const {
            return asset_id;
        }
        uint64_t second_key() const {
            return account.value;
        }
    };
    typedef multi_index<name("tools"), stake_t, indexed_by<"account"_n, const_mem_fun<stake_t, uint64_t, &stake_t::second_key>>> stake_s;


    void addstake (name from, std::vector <uint64_t> asset_ids);
    void unstake(name user, std::vector <uint64_t> asset_ids);

    void addtoken(name user, asset quantity);
   // void rmtoken(name user, asset quantity);

    //uint8_t random(name user, uint64_t asset_id);
    uint64_t get_rand();
};
