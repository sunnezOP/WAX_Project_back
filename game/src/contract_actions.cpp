using namespace std;

void industrialcraft::tokentransfer (name from, name to, asset quantity, string memo){
    require_auth(from);

    if (to != get_self() && from == get_self() && (memo == string("withdraw token " + to.to_string()))) {
        check(memo == string("withdraw token " + to.to_string()), "incorrect memo");
        //rmtoken(to, quantity);
        return;
    }
    if (to == get_self()) {
        check(memo == "deposit", "incorrect memo");
        addtoken(from, quantity);
        return;
    }


}

void industrialcraft::addtoken(name user, asset quantity){
    // get accounts table
    accounts_table accounts(get_self(), get_self().value);
    // check if the user is registered
    auto account = accounts.require_find(user.value, "user is not registered");

    if(quantity.symbol==ICS_SYMBOL){
        accounts.modify(account, same_payer, [&](auto &row) {
            row.ics = account->ics + quantity;
        });
    }else if(quantity.symbol==ICI_SYMBOL){
        accounts.modify(account, same_payer, [&](auto &row) {
            row.ici = account->ici + quantity;
        });
    }else{
        check(quantity.symbol==ICG_SYMBOL, "incorrect token");
        accounts.modify(account, same_payer, [&](auto &row) {
            row.icg = account->icg + quantity;
        });
    }

}

/*void industrialcraft::rmtoken(name user, asset quantity){
    // get accounts table
    accounts_table accounts(get_self(), get_self().value);
    // check if the user is registered
    auto account = accounts.require_find(user.value, "user is not registered");


    if(quantity.symbol==ICS_SYMBOL){
        check(account->ics.amount>=quantity.amount, "incorrect amount of token, you have: " + (account->ics.to_string()));
        accounts.modify(account, same_payer, [&](auto &row) {
            row.ics = account->ics - quantity;
        });
    }else if(quantity.symbol==ICI_SYMBOL){
        check(account->ici.amount>=quantity.amount, "incorrect amount of token, you have: " + (account->ici.to_string()));
        accounts.modify(account, same_payer, [&](auto &row) {
            row.ici = account->ici - quantity;
        });
    }else{
        check(quantity.symbol==ICG_SYMBOL, "incorrect token");
        check(account->icg.amount>=quantity.amount, "incorrect amount of token, you have: " + (account->icg.to_string()));
        accounts.modify(account, same_payer, [&](auto &row) {
            row.icg = account->icg - quantity;
        });
    }

}*/

void industrialcraft::stake(name from, name to, std::vector <uint64_t> asset_ids, std::string memo)
{
    require_auth(from);
    accounts_table accounts(get_self(), get_self().value);


    if (to != get_self() && from == get_self() && (memo == string("unstake " + to.to_string()))) {
        auto account = accounts.require_find(to.value, "user is not registered");
        check(memo == string("unstake " + to.to_string()), "incorrect memo");
        unstake(to, asset_ids);
        return;
    }
    if (to == get_self()) {
        auto account = accounts.require_find(from.value, "user is not registered");

        //check memo
        check(memo == "stake", "incorrect memo");
        addstake(from, asset_ids);
        return;
    }
}


void industrialcraft::addstake(name from, std::vector <uint64_t> asset_ids)
{
    auto self = get_self();

    stake_s stakes = stake_s(get_self(), from.value);
    atomicassets::assets assets_( "atomicassets"_n, get_self().value );

    stucktool_t stucktool(get_self(), get_self().value);
    toolconfig_t toolconf(get_self(), get_self().value);



    //Check all assets_id
    for (uint64_t asset_id : asset_ids){
        auto collection_template_id = assets_.require_find(asset_id, "error incorrect collection t")->template_id;
        auto conftool = toolconf.require_find(collection_template_id, "This collection is not accepted");

        stakes.emplace(get_self(), [&](auto &row) {
            row.account = from;
            row.asset_id = asset_id;
            row.template_id = collection_template_id;
            row.uses = conftool->uses;
            row.availability = true;
        });

        auto toolstuck = stucktool.find(asset_id);

        if (toolstuck == stucktool.end()){
            stuckconfig_t stuckconf(get_self(), get_self().value);
            auto conf = stuckconf.find(collection_template_id);

            stucktool.emplace(get_self(), [&](auto &row) {
                row.asset_id = asset_id;
                row.template_id = collection_template_id;
                row.account = from;
                row.chance = conf->initial;
                row.initial = conf->initial;
                row.increase = conf->increase;
                row.time_hours = conf->time_hours;
                row.time = time_point_sec(0);
                row.availability = true;
            });
        }
        else if (toolstuck->account != from){
            stuckconfig_t stuckconf(get_self(), get_self().value);
            auto conf = stuckconf.find(collection_template_id);

            stucktool.modify(toolstuck, same_payer, [&](auto &row) {
                row.asset_id = asset_id;
                row.template_id = collection_template_id;
                row.account = from;
                row.chance = conf->initial;
                row.initial = conf->initial;
                row.increase = conf->increase;
                row.time_hours = conf->time_hours;
                row.time = time_point_sec(0);
                row.availability = true;
            });
        }
    }
}

void industrialcraft::unstake(name user, std::vector <uint64_t> asset_ids){
    require_auth(get_self());

    stake_s st(get_self(), user.value);

    stucktool_t stucktool(get_self(), get_self().value);
    toolconfig_t toolconf(get_self(), get_self().value);

    for (uint64_t asset_id : asset_ids){
        auto itr = st.require_find(asset_id, "error incorrect asset_id");
        check(user==(itr->account), "this tool belongs to another user");
        auto conftool = toolconf.find(itr->template_id);
        check((itr->uses)==conftool->uses, "to withdraw, you need to do a full repair");
        check(itr->availability==true, "the tool is not available for withdrawal");

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

        st.erase(itr);
    }

}

ACTION industrialcraft::delstaket(name user){
	require_auth(get_self());
	
	stake_s st(get_self(), get_self().value);
	auto itr = st.begin();
	while(itr != st.end()){
    		itr = st.erase(itr);
	}
}

ACTION industrialcraft::delreguserst(){
	require_auth(get_self());
	
	accounts_table st(get_self(), get_self().value);
	auto itr = st.begin();
	while(itr != st.end()){
    		itr = st.erase(itr);
	}
}

ACTION industrialcraft::deluserstate(name user){
    require_auth(user);

    userstate st(get_self(), user.value);
    auto itr = st.begin();
    while(itr != st.end()){
        itr = st.erase(itr);
    }
}

ACTION industrialcraft::settoolconf(const int32_t &template_id,
                                    const asset &ics_repair, const asset &ici_repair, const asset &icg_repair,
                                    const asset &mine,
                                    const asset &ics_craft, const asset &ici_craft, const asset &icg_craft,
                                    const uint8_t &uses, const uint32_t &duration) {
    // check for contract auth
    require_auth(get_self());

    // get fruits config table
    toolconfig_t toolconf(get_self(), get_self().value);
    auto conf = toolconf.find(template_id);

    // if there's no config for this fruit
    if (conf == toolconf.end()) {
        toolconf.emplace(get_self(), [&](auto &row) {
            row.template_id = template_id;
            row.ics_repair=ics_repair;
            row.ici_repair=ici_repair;
            row.icg_repair=icg_repair;
            row.mine = mine;
            row.ics_craft = ics_craft;
            row.ici_craft = ici_craft;
            row.icg_craft = icg_craft;
            row.uses = uses;
            row.duration = duration;
        });
    } else {
        // else: modify existing row
        toolconf.modify(conf, same_payer, [&](auto &row) {
            row.template_id = template_id;
            row.ics_repair=ics_repair;
            row.ici_repair=ici_repair;
            row.icg_repair=icg_repair;
            row.mine = mine;
            row.ics_craft = ics_craft;
            row.ici_craft = ici_craft;
            row.icg_craft = icg_craft;
            row.uses = uses;
            row.duration = duration;
        });
    }
}

ACTION industrialcraft::ralltoolconf() {

    // check for contract auth
    require_auth(get_self());

    // get fruits config table
    toolconfig_t toolconfig(get_self(), get_self().value);
    auto itr = toolconfig.begin();
    while(itr != toolconfig.end()){
        itr = toolconfig.erase(itr);
    }
}

ACTION industrialcraft::rtoolconfig(const int32_t &template_id) {

    // check for contract auth
    require_auth(get_self());

    // get fruits config table
    toolconfig_t toolconfig(get_self(), get_self().value);
    auto itr = toolconfig.require_find(template_id, "template_id not found");

    toolconfig.erase(itr);
}

ACTION industrialcraft::setstuckconf(int32_t template_id, uint8_t initial, uint8_t increase, uint8_t time_hours){
    // check for contract auth
    require_auth(get_self());

    // get fruits config table
    stuckconfig_t stuckconf(get_self(), get_self().value);
    auto conf = stuckconf.find(template_id);

    // if there's no config for this fruit
    if (conf == stuckconf.end()) {
        stuckconf.emplace(get_self(), [&](auto &row) {
            row.template_id = template_id;
            row.initial = initial;
            row.increase = increase;
            row.time_hours = time_hours;
        });
    } else {
        // else: modify existing row
        stuckconf.modify(conf, same_payer, [&](auto &row) {
            row.template_id = template_id;
            row.initial = initial;
            row.increase = increase;
            row.time_hours = time_hours;
        });
    }
}

ACTION industrialcraft::rallstuckcon() {

    // check for contract auth
    require_auth(get_self());

    // get fruits config table
    stuckconfig_t stuckconfig(get_self(), get_self().value);
    auto itr = stuckconfig.begin();
    while(itr != stuckconfig.end()){
        itr = stuckconfig.erase(itr);
    }
}

ACTION industrialcraft::rstuckconfig(const int32_t &template_id) {

    // check for contract auth
    require_auth(get_self());

    // get fruits config table
    stuckconfig_t stuckconfig(get_self(), get_self().value);
    auto itr = stuckconfig.require_find(template_id, "template_id not found");

    stuckconfig.erase(itr);
}

ACTION industrialcraft::settokenconf(uint8_t id, string token, uint8_t fee){
    // check for contract auth
    require_auth(get_self());

    // get fruits config table
    tokenconfig_t tokencon(get_self(), get_self().value);
    auto conf = tokencon.find(id);

    // if there's no config for this fruit
    if (conf == tokencon.end()) {
        tokencon.emplace(get_self(), [&](auto &row) {
            row.id = id;
            row.token = token;
            row.fee = fee;
        });
    } else {
        // else: modify existing row
        tokencon.modify(conf, same_payer, [&](auto &row) {
            row.id = id;
            row.token = token;
            row.fee = fee;
        });
    }
}

ACTION industrialcraft::ralltokencon() {

    // check for contract auth
    require_auth(get_self());

    // get fruits config table
    tokenconfig_t tokencon(get_self(), get_self().value);
    auto itr = tokencon.begin();
    while(itr != tokencon.end()){
        itr = tokencon.erase(itr);
    }
}

ACTION industrialcraft::rtokenconfig(uint8_t id) {

    // check for contract auth
    require_auth(get_self());

    // get fruits config table
    tokenconfig_t tokencon(get_self(), get_self().value);
    auto itr = tokencon.require_find(id, "token not found");

    tokencon.erase(itr);
}