#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "monopoly.h" // NEMENIT !!!

PLAYER players[MAX_PLAYERS];
int num_players = 2; // Default number of players
int game_active = 1; // Flag if game is active now

void print_players()
{
    printf("Players:\n");
    for (int i = 0; i < num_players; ++i)
    {
        printf("%d. S: %d, C: %d, JP: %d, IJ: %s\n",players[i].number, players[i].space_number, players[i].cash, players[i].num_jail_pass, players[i].is_in_jail ? "yes" : "no");

        for (int j = 0; j < players[i].num_properties; j++)
        {

            PROPERTY *property = players[i].owned_properties[j];

            int space_index = -1;
            for (int k = 0; k < NUM_SPACES; ++k)
            {
                if (game_board[k].property == property)
                {
                    space_index = k + 1;
                    break;
                }
            }

            if (space_index != -1)
            {
                printf("\t%s\t%d\t%s\tS%d\n",property->name, property->price, property_colors[property->color],space_index);
            }
        }
    }
}

int check_monopoly(PLAYER player, COLOR color)
{
    int properties_of_color = 0;

    for (int i = 0; i < player.num_properties; ++i)
    {
        if (player.owned_properties[i]->color == color)
        {
            properties_of_color++;
        }
    }

    return properties_of_color == 2;
}



void print_board()
{
    printf("Game board:\n");
    for (int i = 0; i < NUM_SPACES; ++i)
    {
        SPACE space = game_board[i];

        printf("%d.\t", i + 1);
        switch (space.type)
        {
            case Property:

                printf("%-17s %d  ", space.property->name, space.property->price);
                printf("%-8s ", property_colors[space.property->color]);

                int owned = 0;
                for (int j = 0; j < MAX_PLAYERS; j++)
                {
                    PLAYER player = players[j];
                    for (int k = 0; k < player.num_properties; ++k)
                    {
                        if (player.owned_properties[k] == space.property)
                        {
                            owned = 1;
                            printf("P%d ", player.number);
                            printf("%-3s\n", check_monopoly(player, space.property->color) ? "yes" : "no");
                            break;
                        }
                    }
                    if (owned)
                    {
                        break;
                    }
                }
                if (!owned)
                {
                    printf("\n");
                }
                break;

            case Start:
            case Free_parking:
            case In_jail:
            case Go_to_jail:
            case Jail_pass:
                printf("%-15s\n", space_types[space.type]);
                break;

            default:
                break;
        }
    }
}

void print_winners()
{
    if (game_active) {
        printf("WINNER: -\n\n");
        return;
    }

    int max_cash = -1;
    int winning_player = -1;
    int draw = 0;

    // Поиск игрока с максимальной суммой денег
    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        if (players[i].cash > max_cash )
        {
            max_cash = players[i].cash;
            winning_player = i;
            draw = 0; // Сброс флага на случай, если был найден новый лидер
        }
        else if (players[i].cash == max_cash)
        {
            draw = 1; // Если найдено два игрока с одинаковыми деньгами
        }
    }

    // Если ничья, учитываем стоимость недвижимости
    if (draw)
    {
        // Поиск игрока с максимальной общей стоимостью
        for (int i = 0; i < MAX_PLAYERS; i++) {

            int total_cash = players[i].cash;

            for (int j = 0; j < players[i].num_properties; j++)
            {
                total_cash += players[i].owned_properties[j]->price;
            }

            if (total_cash > max_cash)
            {
                max_cash = total_cash;
                winning_player = i;
                draw = 0;
            }
            else if (total_cash == max_cash)
            {
                draw = 1; // Опять два игрока с одинаковой стоимостью
            }
        }

    }

    if (!draw)
    {
        printf("WINNER: P%d", winning_player + 1);
    }
    else
    {
        printf("WINNER: ?");
    }
}

void property_action(int player_number, SPACE *space) {

    PROPERTY *property = space->property;

    // Поиск владельца недвижимости
    int owner_number = -1;

    for (int i = 0; i < num_players; i++)
    {
        for (int j = 0; j < players[i].num_properties; j++)
        {
            if (players[i].owned_properties[j] == property)
            {
                owner_number = i;
                break;
            }
        }
        if (owner_number != -1) {
            break;
        }
    }

    // Если недвижимость принадлежит текущему игроку, ничего не делаем
    if (owner_number == player_number)
    {
        return;
    }

    int rent = property->price;

    // Если недвижимость никому не принадлежит
    if (owner_number == -1)
    {
        // Проверка на банкрот
        if (players[player_number].cash <  property->price) {
            players[player_number].cash = 0;
            players[player_number].num_properties = 0;
            game_active = 0;
            return;
        }

        players[player_number].cash -= property->price;
        players[player_number].owned_properties[players[player_number].num_properties] = property;
        players[player_number].num_properties+=1;

        return;
    }

    // Если недвижимость кому-то принадлежит
    if (check_monopoly(players[owner_number], property->color)) {
        rent *= 2; // Удваиваем аренду
    }

    // Проверка на банкрот
    if (players[player_number].cash < rent) {
        players[player_number].cash = 0;
        players[player_number].num_properties = 0;
        game_active = 0;
        return;
    }

    // Вычитаем аренду из баланса текущего игрока и добавляем к балансу владельца
    players[player_number].cash -= rent;
    players[owner_number].cash += rent;
}

void jail_action(int player_number)
{
    // Проверка на банкрот
    if (players[player_number].cash < 1)
    {
        players[player_number].cash = 0;
        players[player_number].num_properties = 0;
        game_active = 0;
        return;
    }

    players[player_number].cash -= 1;
}

void go_to_jail_action(int player_number) {
    if (players[player_number].num_jail_pass > 0)
    {
        players[player_number].num_jail_pass -= 1;
        return;
    }

    // Игрок отправляется в тюрьму
    players[player_number].space_number = 6;
    players[player_number].is_in_jail = 1;
    players[player_number].is_in_jail = 0;
}

int main(int argc, char *argv[]) {
    int sFlag = 0; // Flag to indicate whether to show the board
    int pFlag = 0; // Flag to indicate whether to show player information
    int gFlag = 0; // Flag to indicate whether to show game status

    int option;
    while ((option = getopt(argc, argv, ":n:spg")) != -1) {
        switch (option) {
            case 'n':
                sscanf(optarg, "%d", &num_players);
                break;
            case 's':
                sFlag = 1;
                break;
            case 'p':
                pFlag = 1;
                break;
            case 'g':
                gFlag = 1;
                break;
            default:
                break;
        }
    }

    // Inicializacia
    for (int i = 0; i < num_players; i++) {
        players[i].number = i+1; // Присваиваем номер игрока
        players[i].space_number = 1; // Игрок стартует на первой клетке

        if (num_players == 2)
        {
            players[i].cash = 20; // Начальная сумма денег
        }
        else if (num_players == 3)
        {
            players[i].cash = 18; // Начальная сумма денег
        }
        else if (num_players == 4)
        {
            players[i].cash = 18; // Начальная сумма денег
        }

        players[i].num_jail_pass = 0; // У игрока изначально нет карточек
        players[i].is_in_jail = 0; // Игрок изначально не в тюрьме
        players[i].num_properties = 0; // У игрока изначально нет купленных недвижимостей
    }

    // Startovaci vystup
    print_players();
    print_board();
    print_winners();

    // Game cycle
    int count = 1;
    while(game_active)
    {
        for (int i = 0; i < num_players; ++i)
        {
            // Kocka
            int dice = 0;
            scanf("%d", &dice);

            // Vypis tahu
            printf("R: %d\n",dice);
            printf("Turn: %d\n",count);
            printf("Player on turn: P%d\n\n",i+1);

            int new_space_number = players[i].space_number + dice;

            // Обработка прохождения старта
            if (new_space_number > NUM_SPACES)
            {
                players[i].cash += 2;
                new_space_number = new_space_number % NUM_SPACES;
            }

            SPACE *new_space = &game_board[new_space_number];
            switch (new_space->type)
            {
                case Property:
                    property_action(i, new_space);
                    break;
                case In_jail:
                    jail_action(i);
                    break;
                case Go_to_jail:
                    go_to_jail_action(i);
                    break;
                case Jail_pass:
                    players[i].num_jail_pass+=1;
                    break;
                case Start:
                case Free_parking:
                    break;
                default:
                    break;
            }

            // Завершение цикла если есть банкрот
            if (!game_active)
            {
                break;
            }

            // Обновляем положение игрока
            if (!players[i].is_in_jail)
            {
                players[i].space_number = new_space_number;
            }

            // Vypisy po tahu
            if(sFlag)
            {
                print_board();
            }
            else if (pFlag)
            {
                print_players();
            }
            else if (gFlag)
            {
                print_players();
                print_board();
                print_winners();
            }

            count +=1;
        }
    }

    // Finalovy vystup
    print_players();
    print_board();
    print_winners();

    return 0;
}
