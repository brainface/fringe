/**
 * A dispenser for papers.  It will sell papers to people for money!
 * @author Pinkfish
 * @started Thu May 24 15:51:25 PDT 2001
 */
inherit "/std/room/furniture/commercial";
#include <room/newspaper.h>
#include <money.h>
#include <move_failures.h>

#define BOARD_TAG "newspaper box"

private string _paper;
private int _cost;

void setup() {
   set_name("box");
   set_short("newspaper box");
   add_adjective("box");
   add_help_file("newspaper_box");
   set_allowed_positions(({"sitting", "lying", "kneeling", "meditating"}));
   set_allowed_room_verbs((["sitting" : "sits", "standing" : "stands" ]));
   set_value(900);
   set_commercial_size(1);
   reset_get();
} /* setup() */

/**
 * This sets the current paper that will be dispensed.
 * @param paper the paper to dispense
 */
void set_paper(string paper) {
   string place;
   string* bits;

   if (!paper) {
      return ;
   }
   _paper = paper;

   place = query_money_place();
   set_short(_paper + " box");
   set_long("This is a battered looking metal box that is full of copies of " +
            (lower_case(paper)[0..3] == "the "?"":"the ") +
            paper + ".  There is a door on the front you could pull open "
            "beside which is small white writing saying " +
            MONEY_HAND->money_value_string(_cost, place) + ".\n");
   if (lower_case(_paper)[0..3] == "the ") {
      add_property("determinate", "");
   }
   bits = explode(lower_case(_paper), " ");
   add_adjective(bits);
} /* set_paper() */

/** @ignore yes */
int do_buy() {
   int paper_cost;
   int cost;
   int edition;
   string place;
   object ob;
   int *editions;

   place = query_money_place();
   cost = _cost;
   paper_cost = (NEWSPAPER_HANDLER->query_paper_cost(_paper) * 2) / 3;
   edition = NEWSPAPER_HANDLER->query_last_edition_num(_paper);
   if (!edition) {
      add_failed_mess("There is no edition to buy.\n");
      return 0;
   }

   if (this_player()->query_value_in(place) < cost) {
      add_failed_mess("You do not have enough money to pay for " +
                      _paper + ", you need " +
                      MONEY_HAND->money_value_string(cost, place) +
                      ".\n");
      return 0;
   }

   this_player()->pay_money(MONEY_HAND->create_money_array(cost, place), place);
   adjust_float(cost - paper_cost);

   ob = clone_object("/obj/misc/newspaper");
   ob->set_paper(_paper);
   ob->set_edition(edition);
   if (ob->move(this_player()) != MOVE_OK) {
      ob->move(environment(this_player()));
      write("Unable to move the paper into your inventory, putting it on "
            "the ground.\n");
   }

   editions = this_player()->query_property("Paper " + _paper);
   if (!editions) {
      editions = ({ });
   }
   if (member_array(edition, editions) == -1) {
      editions += ({ edition });
      this_player()->add_property("Paper " + _paper, editions);
      // We only keep track of unique sales.
      NEWSPAPER_HANDLER->add_edition_paper_sold(_paper, edition,
                        NEWSPAPER_HANDLER->query_paper_cost(_paper));
   } else {
      NEWSPAPER_HANDLER->add_edition_revenue(_paper, edition,
                        NEWSPAPER_HANDLER->query_paper_cost(_paper));
   }

   add_succeeded_mess("$N pull$s a newspaper from $D.\n");
   return 1;
} /* do_buy() */

/** @ignore yes */
string query_main_status(int hints) {
   string ret;
   int paper_cost;
   string place;

   place = query_money_place();
   paper_cost = (NEWSPAPER_HANDLER->query_paper_cost(_paper) * 2) / 3;
   ret = the_short() + ":\n";
   ret +=   "$I$6=   Revenue             : " +
          MONEY_HAND->money_value_string(query_revenue(), place) +
          "\n$I$6=   Cost from publisher : " +
          MONEY_HAND->money_value_string(paper_cost, place) +
          "\n$I$6=   Sale price          : " +
          MONEY_HAND->money_value_string(_cost, place) +
          "\n";
   if (hints) {
      ret += "$I$6=      set cost <amount> on <box>\n";
   }
   return ret;
} /* query_main_status() */

/** @ignore yes */
int do_set_cost(string amount) {
   int amt;
   string place;

   place = query_money_place();

   amt = MONEY_HAND->value_from_string(amount, place);
   if (amt <= 0) {
      add_failed_mess("The value " + amount + " is invalid.\n");
      return 0;
   }

   _cost = amt;
   set_long("This is a battered looking metal box that is full of copies of " +
            (lower_case(_paper)[0..3] == "the "?"":"the ") +
            _paper + ".  There is a door on the front you could pull open "
            "beside which is small white writing saying " +
            MONEY_HAND->money_value_string(_cost, place) + ".\n");
   add_succeeded_mess("$N set$s the cost of buy papers from $D to " +
                      MONEY_HAND->money_value_string(amt, place) + ".\n");
   return 1;
} /* do_set_cost() */

void init() {
   add_command("buy", "paper from <direct:object>", (: do_buy() :));
   add_command("pull", "[door] [on] <direct:object>", (: do_buy() :));
   add_command("pull", "open <direct:object>", (: do_buy() :));

   if (is_allowed(this_player()->query_name())) {
      add_command("set", "cost <string'amount'> on <direct:object>",
                  (: do_set_cost($4[0]) :));
   }

   ::init();
} /* init() */

/** @ignore yes */
mapping query_commercial_options() {
   mapping ret;
   string paper;

   ret = ([ ]);
   foreach (paper in NEWSPAPER_HANDLER->query_all_papers()) {
      ret[paper] = 0;
   }
   return ([ "paper" : ret ]);
} /* query_commercial_options() */

/** @ignore yes */
void set_commercial_option(string type, string name) {
   switch (type) {
   case "paper" :
      _cost = NEWSPAPER_HANDLER->query_paper_cost(name);
      call_out("set_paper", 1, name);
      break;
   }
} /* set_commercial_option() */

/** @ignore yes */
mapping query_dynamic_auto_load() {
   mapping map;

   map = commercial::query_dynamic_auto_load();
   add_auto_load_value(map, BOARD_TAG, "cost", _cost);
   add_auto_load_value(map, BOARD_TAG, "paper", _paper);
   return map;
} /* query_dynamic_auto_load() */

/** @ignore yes */
void init_dynamic_arg(mapping map, object player) {
   string name;

   commercial::init_dynamic_arg(map, player);

   _cost = query_auto_load_value(map, BOARD_TAG, "cost");
   name = query_auto_load_value(map, BOARD_TAG, "paper");
   call_out("set_paper", 1, name);
} /* init_dynamic_arg() */
