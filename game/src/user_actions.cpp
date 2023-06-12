using namespace std;
/*
    Register a new user and save them in accounts table
*/
ACTION industrialcraft::regnewuser(const name &user) {
    // check for caller auth
    require_auth(user);

    // get accounts table
    accounts_table accounts(get_self(), get_self().value);
    auto account = accounts.find(user.value);

    // check if the account is not already registered
    check(account == accounts.end(), "user already registered");

    // emplace a new user row
    accounts.emplace(user, [&](auto &row) {
        row.account = user;
        row.pickaxe = 2;
        row.drill = 2;
        row.mine_machine = 2;
        row.ics = asset(0, ICS_SYMBOL);
        row.ici = asset(0, ICI_SYMBOL);
        row.icg = asset(0, ICG_SYMBOL);
    });
}


ACTION industrialcraft::withdrawtool(const name &user,const std::vector <uint64_t> &assets_id){
    require_auth(user);


    action({get_self(), name("active")},
           name("atomicassets"),
           name("transfer"),
           make_tuple(
                   get_self(),
                   user,
                   assets_id,
                   string("unstake " + user.to_string()))).send();

}

ACTION industrialcraft::withdraw(const name &user, asset quantity){
    require_auth(user);

    tokenconfig_t tokenconf(get_self(), get_self().value);
    float fee;

    // get accounts table
    accounts_table accounts(get_self(), get_self().value);
    // check if the user is registered
    auto account = accounts.require_find(user.value, "user is not registered");

    if (quantity.symbol == ICS_SYMBOL){
        fee = tokenconf.find(1)->fee * 1.00 / 100;

        check(account->ics.amount>=quantity.amount, "incorrect amount of token, you have: " + (account->ics.to_string()));
        accounts.modify(account, same_payer, [&](auto &row) {
            row.ics = account->ics - quantity;
        });
    }else if (quantity.symbol == ICI_SYMBOL){
        fee = tokenconf.find(2)->fee * 1.00 / 100;

        check(account->ici.amount>=quantity.amount, "incorrect amount of token, you have: " + (account->ici.to_string()));
        accounts.modify(account, same_payer, [&](auto &row) {
            row.ici = account->ici - quantity;
        });
    }else{
        check (quantity.symbol == ICG_SYMBOL, "incorrect token");
        fee = tokenconf.find(3)->fee * 1.00 / 100;

        check(account->icg.amount>=quantity.amount, "incorrect amount of token, you have: " + (account->icg.to_string()));
        accounts.modify(account, same_payer, [&](auto &row) {
            row.icg = account->icg - quantity;
        });
    }

    uint64_t wfee = quantity.amount;
    quantity.amount = quantity.amount - (quantity.amount * fee);
    wfee = wfee - quantity.amount;

    action({get_self(), name("active")},
           TOKEN_CONTRACT,
           name("transfer"),
           make_tuple(
                   get_self(),
                   user,
                   quantity,
                   string("withdraw token " + user.to_string()))).send();

    action(permission_level{get_self(), name("active")},
           TOKEN_CONTRACT,
           name("retire"),
           make_tuple(
                   asset(wfee, quantity.symbol),
                   std::string("withdraw fee " + user.to_string()))).send();

}

ACTION industrialcraft::farm(const name &user,const uint64_t &asset_id){
    require_auth(user);

    accounts_table accounts(get_self(), get_self().value);

    // check if the user is registered
    auto account = accounts.require_find(user.value, "user is not registered");

    stake_s stakes = stake_s(get_self(), user.value);
    auto tool = stakes.require_find(asset_id, "first you need to deposit this tool");
    auto template_id = tool->template_id;

    check(tool->uses > 0, "tool is broken "  + to_string(asset_id));
    check(tool->availability==true, "tool is already in use " + to_string(asset_id));

    stucktool_t stucktool(get_self(), get_self().value);
    auto toolstuck = stucktool.find(asset_id);
    bool f = true;
    if(toolstuck->availability==false){
        f = false;
        if((toolstuck->time_hours*60) <= (current_time_point().sec_since_epoch() - (toolstuck->time).sec_since_epoch())) {
            stucktool.modify(toolstuck, same_payer, [&](auto &row) {
                row.availability = true;
            });
            f = true;
        }
    }
    check(f==true, "tool in stuck " + to_string(asset_id));

    userstate state(get_self(), user.value);
    if(template_id==Common_Pickaxe || template_id==Rare_Pickaxe || template_id==Legendary_Pickaxe || template_id==Industrial_Pickaxe) {
        check(account->pickaxe > 0, "all farm sites in use");
        state.emplace(user, [&](auto &row) {
            row.type = pickaxe;
            row.assets_id = asset_id;
            row.time = time_point_sec(current_time_point().sec_since_epoch());
        });
        stakes.modify(tool, same_payer, [&](auto &row) {
            row.uses = tool->uses - 1;
            row.availability = false;
        });
        accounts.modify(account, same_payer, [&](auto &row) {
            row.pickaxe = account->pickaxe - 1;
        });
        return;
    }

    if(template_id==Common_Drill || template_id==Rare_Drill || template_id==Legendary_Drill || template_id==Industrial_Drill) {
        check(account->drill > 0, "all farm sites in use");
        state.emplace(user, [&](auto &row) {
            row.type = drill;
            row.assets_id = asset_id;
            row.time = time_point_sec(current_time_point().sec_since_epoch());
        });
        stakes.modify(tool, same_payer, [&](auto &row) {
            row.uses = tool->uses - 1;
            row.availability = false;
        });
        accounts.modify(account, same_payer, [&](auto &row) {
            row.drill = account->drill - 1;
        });
        return;
    }

    if(template_id==Common_Mine_Machine || template_id==Rare_Mine_Machine || template_id==Legendary_Mine_Machine || template_id==Industrial_Mine_Machine) {
        check(account->mine_machine > 0, "all farm sites in use");
        state.emplace(user, [&](auto &row) {
            row.type = mine_machine;
            row.assets_id = asset_id;
            row.time = time_point_sec(current_time_point().sec_since_epoch());
        });
        stakes.modify(tool, same_payer, [&](auto &row) {
            row.uses = tool->uses - 1;
            row.availability = false;
        });
        accounts.modify(account, same_payer, [&](auto &row) {
            row.mine_machine = account->mine_machine - 1;
        });
        return;
    }
}

ACTION industrialcraft::unfarm(const name &user,const uint64_t &asset_id){
    require_auth(user);
    // get accounts table
    accounts_table accounts(get_self(), get_self().value);

    // check if the user is registered
    auto account = accounts.require_find(user.value, "user is not registered");

    userstate state(get_self(), user.value);
    auto toolstate = state.require_find(asset_id, "this tool does not farm");

    stake_s stakes = stake_s(get_self(), user.value);
    auto tool = stakes.require_find(asset_id, "first you need to deposit this tool");
    auto template_id = tool->template_id;

    toolconfig_t toolconf(get_self(), get_self().value);
    auto conf = toolconf.find(template_id);

    check((conf->duration * 60) <= (current_time_point().sec_since_epoch() - (toolstate->time).sec_since_epoch()), "still in progress " + to_string((conf->duration * 60) - (current_time_point().sec_since_epoch() - (toolstate->time).sec_since_epoch())) + " sec");

    stucktool_t stucktool(get_self(), get_self().value);
    auto toolstuck = stucktool.find(asset_id);

    uint8_t rand = get_rand()%100+1;
    if (rand <= toolstuck->chance){
        stucktool.modify(toolstuck, same_payer, [&](auto &row) {
            row.chance = toolstuck->initial;
            row.time = time_point_sec(current_time_point().sec_since_epoch());
            row.availability = false;
        });
    }else{
        uint8_t randincrease = get_rand()%1000+1;
        if(randincrease<=(toolstuck->increase*10)) {
            uint8_t chance = 0;
            if ((toolstuck->chance + toolstuck->initial) > 100) { chance = 100; } else { chance = toolstuck->chance + toolstuck->initial; }
            stucktool.modify(toolstuck, same_payer, [&](auto &row) {
                row.chance = chance;
                row.availability = true;
            });
        }
    }

    state.erase(toolstate);
    if(template_id==Common_Pickaxe || template_id==Rare_Pickaxe || template_id==Legendary_Pickaxe || template_id==Industrial_Pickaxe) {
        auto ics = account->ics + conf->mine;
        accounts.modify(account, same_payer, [&](auto &row) {
            row.ics = ics;
            row.pickaxe = account->pickaxe + 1;
        });
        stakes.modify(tool, same_payer, [&](auto &row){
            row.availability = true;
        });
        action(permission_level{get_self(), name("active")}, TOKEN_CONTRACT, name("issue"),
               make_tuple(get_self(), conf->mine, std::string("Farming " + to_string(template_id) + " - " + user.to_string())))
                .send();
        return;
    }

    if(template_id==Common_Drill || template_id==Rare_Drill || template_id==Legendary_Drill || template_id==Industrial_Drill) {
        auto ici = account->ici + conf->mine;
        accounts.modify(account, same_payer, [&](auto &row) {
            row.ici = ici;
            row.drill = account->drill + 1;
        });
        stakes.modify(tool, same_payer, [&](auto &row){
            row.availability = true;
        });
        action(permission_level{get_self(), name("active")}, TOKEN_CONTRACT, name("issue"),
               make_tuple(get_self(), conf->mine, std::string("Farming " + to_string(template_id) + " - " + user.to_string())))
                .send();
        return;
    }

    if(template_id==Common_Mine_Machine || template_id==Rare_Mine_Machine || template_id==Legendary_Mine_Machine || template_id==Industrial_Mine_Machine) {
        auto icg = account->icg + conf->mine;
        accounts.modify(account, same_payer, [&](auto &row) {
            row.icg = icg;
            row.mine_machine = account->mine_machine + 1;
        });
        stakes.modify(tool, same_payer, [&](auto &row){
            row.availability = true;
        });
        action(permission_level{get_self(), name("active")}, TOKEN_CONTRACT, name("issue"),
               make_tuple(get_self(), conf->mine, std::string("Farming " + to_string(template_id) + " - " + user.to_string())))
                .send();
        return;
    }

}

ACTION industrialcraft::fullrepair(const name &user,const uint64_t &asset_id){
    require_auth(user);

    // get accounts table
    accounts_table accounts(get_self(), get_self().value);
    // check if the user is registered
    auto account = accounts.require_find(user.value, "user is not registered");

    stake_s stakes = stake_s(get_self(), user.value);
    auto tool = stakes.require_find(asset_id, "first you need to deposit this tool");
    auto template_id = tool->template_id;

    stucktool_t stucktool(get_self(), get_self().value);
    auto toolstuck = stucktool.find(asset_id);

    toolconfig_t toolconf(get_self(), get_self().value);
    auto conf = toolconf.find(template_id);

    bool f = true;
    if(toolstuck->availability==false){
        f = false;
        if((toolstuck->time_hours*60) <= (current_time_point().sec_since_epoch() - (toolstuck->time).sec_since_epoch())) {
            stucktool.modify(toolstuck, same_payer, [&](auto &row) {
                row.availability = true;
            });
            f = true;
        }
    }

    check(tool->availability==true, "tool is already in use");
    check(f==true, "tool stuck " + to_string(asset_id));
    check((tool->uses)<conf->uses, "don`t need to repair");

    check(account->ics>=conf->ics_repair, "insufficient funds");
    check(account->ici>=conf->ici_repair, "insufficient funds");
    check(account->icg>=conf->icg_repair, "insufficient funds");

    stakes.modify(tool, same_payer, [&](auto &row){
        row.uses = conf->uses;
    });

    accounts.modify(account, same_payer, [&](auto &row) {
        row.ics = account->ics - conf->ics_repair;
        row.ici = account->ici - conf->ici_repair;
        row.icg = account->icg - conf->icg_repair;
    });

    if(conf->ics_repair.amount > 0)
    action(permission_level{get_self(), name("active")}, TOKEN_CONTRACT, name("retire"),
           make_tuple(conf->ics_repair,
                      std::string("Full Repair " + to_string(template_id) + " - " + user.to_string())))
            .send();

    if(conf->ici_repair.amount > 0)
    action(permission_level{get_self(), name("active")}, TOKEN_CONTRACT, name("retire"),
           make_tuple(conf->ici_repair,
                      std::string("Full Repair " + to_string(template_id) + " - " + user.to_string())))
            .send();

    if(conf->icg_repair.amount > 0)
    action(permission_level{get_self(), name("active")}, TOKEN_CONTRACT, name("retire"),
           make_tuple(conf->icg_repair,
                      std::string("Full Repair " + to_string(template_id) + " - " + user.to_string())))
            .send();
}

ACTION industrialcraft::craft(const name &schema_name, const int32_t &template_id, const name &new_asset_owner){
    require_auth(new_asset_owner);

    // get accounts table
    accounts_table accounts(get_self(), get_self().value);
    // check if the user is registered
    auto account = accounts.require_find(new_asset_owner.value, "user is not registered");

    toolconfig_t toolconf(get_self(), get_self().value);
    auto conftool = toolconf.require_find(template_id, "incorrect template_id");

    check(account->ics.amount>=conftool->ics_craft.amount, "insufficient funds, you need: " + conftool->ics_craft.to_string());
    check(account->ici.amount>=conftool->ici_craft.amount, "insufficient funds, you need: " + conftool->ici_craft.to_string());
    check(account->icg.amount>=conftool->icg_craft.amount, "insufficient funds, you need: " + conftool->icg_craft.to_string());

    accounts.modify(account, same_payer, [&](auto &row) {
        row.ics = account->ics - conftool->ics_craft;
        row.ici = account->ici - conftool->ici_craft;
        row.icg = account->icg - conftool->icg_craft;
    });

    action(permission_level{get_self(), name("active")},
           "atomicassets"_n,
           name("mintasset"),
           make_tuple(
                   get_self(),
                   collection_name,
                   schema_name,
                   template_id,
                   new_asset_owner,
                   std::map<std::string, atomicassets::ATOMIC_ATTRIBUTE>(),
                   std::map<std::string, atomicassets::ATOMIC_ATTRIBUTE>(),
                   std::vector<asset>())).send();

    if(conftool->ics_craft.amount > 0)
        action(permission_level{get_self(), name("active")}, TOKEN_CONTRACT, name("retire"),
               make_tuple(conftool->ics_craft,
                          std::string("Craft " + to_string(template_id) + " - " + new_asset_owner.to_string())))
                .send();

    if(conftool->ici_craft.amount > 0)
        action(permission_level{get_self(), name("active")}, TOKEN_CONTRACT, name("retire"),
               make_tuple(conftool->ici_craft,
                          std::string("Craft " + to_string(template_id) + " - " + new_asset_owner.to_string())))
                .send();

    if(conftool->icg_craft.amount > 0)
        action(permission_level{get_self(), name("active")}, TOKEN_CONTRACT, name("retire"),
               make_tuple(conftool->icg_craft,
                          std::string("Craft " + to_string(template_id) + " - " + new_asset_owner.to_string())))
                .send();

}

ACTION industrialcraft::randtest(){
    check(0==1, get_rand()%1000+1);
}