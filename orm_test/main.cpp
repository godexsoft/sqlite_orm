//
//  main.cpp
//  orm_test
//
//  Created by Alex Kremer on 07/11/2012.
//  Copyright (c) 2012 godexsoft. All rights reserved.
//

#include <iostream>
#include "sqlite_orm.h"

class user;

#undef  MODEL_NAME
#define MODEL_NAME score
BEGIN_MODEL_DECLARATION()
: highscore(0)
{
}

static const std::string table_name()
{
    return "scores";
}

FIELD_INT(highscore);
BELONGS_TO(user, user);

END_DECLARATION();

#undef  MODEL_NAME
#define MODEL_NAME user
BEGIN_MODEL_DECLARATION()
: name("Fernando")
{
}

static const std::string table_name()
{
    return "users";
}

FIELD_STR(name);
HAS_MANY(score, scores);

END_DECLARATION();

BELONGS_TO_IMPL(score, user, user);
HAS_MANY_IMPL(user, score, scores);

int main(int argc, const char * argv[])
{
    sqlite3pp::database db_("test.db");
    sqlite::orm::dao<user> user_dao(db_);
    sqlite::orm::dao<score> score_dao(db_);
    
    srand(static_cast<unsigned int>(time(NULL)));
    
    bool want_clean_db = false;
    
    if(want_clean_db)
    {
        std::cout << "Drop everything from DB...\n";
        db_.execute("DELETE FROM users");
        db_.execute("DELETE FROM scores");
    }
    
    std::cout << "Now try to add a user...\n";
    
    user usr1;
    usr1.set_name("Fernando Palha");
    
    // add some scores
    score score1;
    score1.set_highscore(123456 + rand()%100);
    usr1.add_to_scores(score1);
    
    score score2;
    score2.set_highscore(3456789 + rand()%100);
    usr1.add_to_scores(score2);

    // persist the user
    user_dao.save(usr1);
    
    std::cout << "User saved.\n";
    
    // list users
    std::vector<boost::shared_ptr<user> > users = user_dao.query_all();
    
    for(std::vector<boost::shared_ptr<user> >::iterator it = users.begin();
        it != users.end(); ++it)
    {
        std::cout << "* User: [" << (*it)->get_id() << "] " << (*it)->get_name() << "\n";
        
        // fetch his scores (lazy loading) and print them
        std::vector<boost::shared_ptr<score> > scores = (*it)->fetch_scores();

        for(std::vector<boost::shared_ptr<score> >::iterator jt = scores.begin();
            jt != scores.end(); ++jt)
        {
            std::cout << "   * Score: " << (*jt)->get_highscore() << "\n";
        }
    }
    
    return 0;
}

