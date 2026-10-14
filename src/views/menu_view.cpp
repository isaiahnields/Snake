#include "menu_view.h"

#include "../renderer.h"
#include "../view_manager.h"

#include <utility>

MenuItem menu_link(const std::string &label, MenuPage target)
{
    return {label, target};
}

MenuItem menu_command(const std::string &label, MenuPage page, MenuAction action)
{
    return {label, page, action};
}

MenuItem menu_unavailable(const std::string &label, MenuPage page)
{
    return {label + " [Not implemented]", page, MenuAction::None, false};
}

MenuView::MenuView(MenuPage initial_page) : page(initial_page) {}

std::vector<std::string> MenuView::details() const
{
    return {};
}

int MenuView::shortcut_number(std::size_t index) const
{
    return static_cast<int>(index) + 1;
}

void MenuView::open(MenuPage destination, const std::string &message)
{
    page = destination;
    selection = 0;
    notice = message;
}

void MenuView::move(int offset)
{
    const int count = static_cast<int>(items().size());
    selection = ((selection + offset) % count + count) % count;
}

void MenuView::back(ViewManager &manager)
{
    navigate(items().back().target, manager);
}

void MenuView::activate(ViewManager &manager)
{
    const auto entries = items();
    const auto &item = entries.at(selection);
    if (!item.available)
    {
        notice = "Not implemented yet. No changes were made.";
        return;
    }
    if (item.action != MenuAction::None)
    {
        perform(item.action, manager);
        return;
    }
    navigate(item.target, manager);
}

void MenuView::shortcut(int number, ViewManager &manager)
{
    const auto count = items().size();
    for (std::size_t index = 0; index < count; ++index)
    {
        if (shortcut_number(index) != number) continue;
        selection = static_cast<int>(index);
        activate(manager);
        return;
    }
}

void MenuView::handle_event(ViewManager &manager, const SDL_Event &event)
{
    if (event.type != SDL_EVENT_KEY_DOWN) return;

    const SDL_Keycode key = event.key.key;
    if (key == SDLK_ESCAPE)
        back(manager);
    else if (key == SDLK_UP || key == SDLK_W)
        move(-1);
    else if (key == SDLK_DOWN || key == SDLK_S)
        move(1);
    else if (key == SDLK_RETURN || key == SDLK_SPACE)
        activate(manager);
    else if (key >= SDLK_0 && key <= SDLK_9)
        shortcut(static_cast<int>(key - SDLK_0), manager);
}

void MenuView::render(SDL_Renderer *renderer) const
{
    clicked_item_ = draw_menu(renderer, *this);
}

void MenuView::apply_ui_action(ViewManager &manager)
{
    const int clicked = std::exchange(clicked_item_, -1);
    if (clicked < 0) return;
    selection = clicked;
    activate(manager);
}

MenuView *MenuView::as_menu()
{
    return this;
}

void MenuView::perform(MenuAction action, ViewManager &manager)
{
    (void)action;
    (void)manager;
}

void MenuView::navigate(MenuPage target, ViewManager &manager)
{
    if (owns(target))
        open(target);
    else
        manager.open_page(target);
}
