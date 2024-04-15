#include "game.hpp"

void game::incr_lb_points(const name &lbname, const name &account, uint64_t points)
{
    lboards_t lboard(get_self(), lbname.value);

    auto account_itr = lboard.find(account.value);

    if (account_itr == std::end(lboard))
    {
        lboard.emplace(get_self(), [&](auto &new_row)
                       {
            new_row.account = account;
            new_row.points = points; });
    }
    else
    {
        lboard.modify(account_itr, get_self(), [&](auto &row)
                      { row.points += points; });
    }
}

void game::decr_lb_points(const name &lbname, const name &account, uint64_t points)
{
    lboards_t lboard(get_self(), lbname.value);

    auto account_itr = lboard.require_find(account.value, "Account not found in leaderboard");

    lboard.modify(account_itr, get_self(), [&](auto &row)
                  { row.points -= points; });
}

void game::set_lb_points(const name &lbname, const name &account, uint64_t points)
{
    lboards_t lboard(get_self(), lbname.value);

    auto account_itr = lboard.find(account.value);

    if (account_itr == std::end(lboard))
    {
        lboard.emplace(get_self(), [&](auto &new_row)
                       {
            new_row.account = account;
            new_row.points = points; });
    }
    else
    {
        lboard.modify(account_itr, get_self(), [&](auto &row)
                      { row.points = points; });
    }
}

float game::get_mining_power(const uint64_t asset_id, const std::map<std::string, uint32_t> &stats)
{
    auto assets = atomicassets::get_assets(get_self());
    auto assets_itr = assets.require_find(asset_id, "asset not found");

    auto item_mdata = get_mdata(assets_itr);
    auto item_template_idata = get_template_idata(assets_itr->template_id, assets_itr->collection_name);

    const float &miningRate = std::get<float>(item_template_idata["miningRate"]);
    
    uint8_t current_lvl = 1;

    if (item_mdata.find("level") != std::end(item_mdata))
    {
        current_lvl = std::get<uint8_t>(item_mdata["level"]);
    }

    const uint8_t upgrade_percentage = 2 + stats.at("vitality") / 10.0f;

    float miningRate_according2lvl = miningRate + stats.at("productivity") / 10.0f;
    for (uint8_t i = 1; i < current_lvl; ++i)
        miningRate_according2lvl = miningRate_according2lvl + (miningRate_according2lvl * upgrade_percentage / 100);

    return miningRate_according2lvl;
}

void game::update_mining_power_lb(const name &account)
{
    staked_t staked_table(get_self(), account.value);
    float mining_power = 0.0f;

    const std::map<std::string, uint32_t> stats = get_stats(account);
    auto assets = atomicassets::get_assets(get_self());

    for (const auto &staked : staked_table)
    {
        auto farmingitem_itr = assets.find(staked.asset_id);
        auto farmingitem_mdata = get_mdata(farmingitem_itr);

        float miningBoost = 1;
        if (farmingitem_mdata.find("miningBoost") != std::end(farmingitem_mdata))
            miningBoost = std::get<float>(farmingitem_mdata["miningBoost"]);

        for (const uint64_t asset_id : staked.staked_items)
        {
            mining_power += get_mining_power(asset_id, stats);
        }
    }

    set_lb_points("miningpwr"_n, account, mining_power);
}
