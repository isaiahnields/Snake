#include "hall_of_fame_view.h"

namespace
{
std::string scores_line(const std::vector<int> &scores, const std::string &players)
{
    if (scores.empty()) return "No " + players + " games completed this session.";
    return "Best " + players + " scores this session: " + format_scores(scores);
}
} // namespace

HallOfFameView::HallOfFameView(const SessionState &session)
    : MenuView(MenuPage::HallOfFame), session_(session)
{
}

bool HallOfFameView::owns(MenuPage candidate) const
{
    return candidate == MenuPage::HallOfFame || candidate == MenuPage::ManualGames ||
           candidate == MenuPage::AIGames;
}

std::string HallOfFameView::title() const
{
    switch (page)
    {
    case MenuPage::ManualGames:
        return "TOP MANUAL GAMES";
    case MenuPage::AIGames:
        return "TOP AI GAMES";
    default:
        return "HALL OF FAME (BEST GAMES & METRICS)";
    }
}

std::vector<std::string> HallOfFameView::details() const
{
    switch (page)
    {
    case MenuPage::ManualGames:
        return {scores_line(session_.best_manual_scores, "manual"),
                "Scores are kept in memory only; saved leaderboards are not implemented.",
                "Planned metrics: time survived, total moves, moves/apple.",
                "Watch Replay requires the recording system."};
    case MenuPage::AIGames:
        return {scores_line(session_.best_ai_scores, "AI test"),
                "Scores are kept in memory only; saved leaderboards are not implemented.",
                "Planned metrics: grid coverage, generation, compute time.",
                "Watch Replay requires the recording system."};
    default:
        return {};
    }
}

std::vector<MenuItem> HallOfFameView::items() const
{
    if (page == MenuPage::HallOfFame)
        return {menu_link("Top Manual Games", MenuPage::ManualGames),
                menu_link("Top AI Games", MenuPage::AIGames),
                menu_link("Return to Main Menu", MenuPage::Main)};
    return {menu_link("Return to Hall of Fame", MenuPage::HallOfFame)};
}
