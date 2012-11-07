//
//  sqlite_orm.cpp
//  sqlite_orm
//
//  Created by Alex Kremer on 30/11/2011.
//  Copyright (c) 2011 godexsoft. All rights reserved.
//

#include "sqlite_orm.h"

namespace sqlite {
namespace orm {
    
    template<>
    const std::string wrap_type(std::string v)
    {
        return v;
    }
    
    template<>
    const std::string wrap_type(sql_date v)
    {
        return "DATETIME('" + v.to_string() + "')";
    }
    
    template<>
    const std::string wrap_type(bool v)
    {
        return v?"1":"0";
    }
    
    template<>
    const std::string wrap_type(long v)
    {
        return boost::lexical_cast<std::string>(v);
    }
    
    template<>
    const std::string wrap_type(int v)
    {
        return boost::lexical_cast<std::string>(v);
    }
    
    template<>
    const std::string wrap_type(sqlite3_int64 v)
    {
        return boost::lexical_cast<std::string>(v);
    }
    
    template<>
    const std::string wrap_type(double v)
    {
        return boost::lexical_cast<std::string>(v);
    }
        
}; // orm
}; // sqlite
