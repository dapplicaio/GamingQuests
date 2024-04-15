#include "game.hpp"

void game::claim(const name &owner, const uint64_t &farmingitem)
{
    require_auth(owner);

    staked_t staked_table(get_self(), owner.value);
    auto staked_table_itr = staked_table.require_find(farmingitem, "Could not find staked farming item");
    auto assets = atomicassets::get_assets(get_self());
    auto assets_itr = assets.find(farmingitem);

    // to get mining boost
    auto farmingitem_mdata = get_mdata(assets_itr);
    float miningBoost = 1;
    if (farmingitem_mdata.find("miningBoost") != std::end(farmingitem_mdata))
        miningBoost = std::get<float>(farmingitem_mdata["miningBoost"]);

    std::map<std::string, uint32_t> stats = get_stats(owner);

    // first - resource name, second - resource amount
    std::map<std::string, float> mined_resources;
    const uint32_t &time_now = current_time_point().sec_since_epoch();
    for (const uint64_t &item_to_collect : staked_table_itr->staked_items)
    {
        auto assets_itr = assets.find(item_to_collect);
        const uint8_t upgrade_percentage = 2 + stats["vitality"] / 10.0f;

        const std::pair<std::string, float> item_reward = claim_item(assets_itr, upgrade_percentage, time_now, stats);

        if (item_reward != std::pair<std::string, float>())
            if (item_reward.second > 0)
                mined_resources[item_reward.first] += item_reward.second;
    }
    check(mined_resources.size() > 0, "Nothing to claim");

    increase_owner_resources_balance(owner, mined_resources);
    
    for (const auto& resource_amount_pair : mined_resources)
    {
        update_quests(owner, resource_amount_pair.first, resource_amount_pair.second);
    }
}

const std::pair<std::string, float> game::claim_item(atomicassets::assets_t::const_iterator &assets_itr, const uint8_t &upgrade_percentage, const uint32_t &time_now, const std::map<std::string, uint32_t> &stats)
{
    auto item_mdata = get_mdata(assets_itr);
    const uint32_t &lastClaim = std::get<uint32_t>(item_mdata["lastClaim"]);
    std::pair<std::string, float> mined_resource;

    if (time_now > lastClaim)
    {
        auto item_template_idata = get_template_idata(assets_itr->template_id, assets_itr->collection_name);
        const float &miningRate = std::get<float>(item_template_idata["miningRate"]);
        const std::string &farmResource = std::get<std::string>(item_template_idata["farmResource"]);
        const uint8_t &current_lvl = std::get<uint8_t>(item_mdata["level"]);

        // calculate mining rate according to lvl
        float miningRate_according2lvl = miningRate + stats.at("productivity") / 10.0f;
        for (uint8_t i = 1; i < current_lvl; ++i)
            miningRate_according2lvl = miningRate_according2lvl + (miningRate_according2lvl * upgrade_percentage / 100);

        const float &reward = (time_now - lastClaim) * miningRate_according2lvl;
        item_mdata["lastClaim"] = time_now;
        update_mdata(assets_itr, item_mdata, get_self());

        mined_resource.first = farmResource;
        mined_resource.second = reward;
    }

    return mined_resource;
}
