//
//  sqlite_orm.h
//  sqlite_orm
//
//  Created by Alex Kremer on 30/11/2011.
//  Copyright (c) 2011 godexsoft. All rights reserved.
//

#pragma once
#ifndef _SQLITE_ORM_H_
#define _SQLITE_ORM_H_

// #define DEBUG_SQL 1

#include <iostream>
#include <vector>
#include <boost/shared_ptr.hpp>
#include <boost/lexical_cast.hpp>
#include <sstream>
#include <algorithm>
#include <map>

#include <boost/any.hpp>
#include <boost/bind.hpp>

#include "sqlite3pp.h"

namespace sqlite {
namespace orm {
    
    using namespace sqlite3pp;
    
    template<typename T>
    const std::string wrap_type(T v);
    
    class base_dao
    {
    };
    
    struct sql_date
    {
        sqlite3_int64 value;
        std::string svalue;
        
        sql_date(sqlite3_int64 v)
        : value(v)
        {}
        
        sql_date(const std::string& v)
        : svalue(v)
        {}
        
        operator sqlite3_int64() { return value; }

        const std::string to_string() const
        {
            if(svalue.empty())
            {
                return boost::lexical_cast<std::string>(value);
            }
            return svalue;
        }
    };
    
    struct base_model;
    
    struct base_field
    {
        boost::any def;
        
        template<typename T>
        base_field(const T& a)
        : def(a)
        {
        }
        
        virtual const std::string get_name() = 0;
        virtual const std::string get_type() = 0;
    };
    
    struct base_foreign_collection
    {
        int offset;
        virtual void save(base_model&) = 0;
        virtual void remove(base_model&) = 0;
    };
    
    template<typename T>
    struct foreign_collection
    : public base_foreign_collection
    {
        std::vector<T> collection;
        void save(base_model&) = 0;
        void remove(base_model&) = 0;
    };
    
    template<typename T>
    class dao
    : public base_dao
    , public T
    {
    private:
        void init(database& db)
        {
            static bool _init = false;
            if(!_init)
            {
                // Database is set only once
                db_ = &db;
                
                // Create this table in DB
                std::string q = "CREATE TABLE IF NOT EXISTS ";
                q.append(T::table_name()).append(" (id__ INTEGER PRIMARY KEY AUTOINCREMENT, ");
                for(std::vector<sqlite::orm::base_field*>::iterator it = T::fields_.begin();
                    it != T::fields_.end(); ++it)
                {
                    q.append((*it)->get_name());
                    q.append(" ").append((*it)->get_type());
                    if(it+1 != T::fields_.end())
                    {
                        q.append(", ");
                    }
                }
                q.append(")");
                
#ifdef DEBUG_SQL
                std::cout << q << "\n";
#endif
                db_->execute(q.c_str());
                
                _init = true;
            }
        }
        
        static boost::any get_field(query::rows::getstream& getter, std::vector<sqlite::orm::base_field*>::iterator& it)
        {
            if((*it)->def.type() == typeid(long))
            {
                sqlite3_int64 val;
                getter >> val;
                return boost::any(val);
            }
            else if((*it)->def.type() == typeid(int))
            {
                int val;
                getter >> val;
                return boost::any(val);
            }
            else if((*it)->def.type() == typeid(sqlite3_int64))
            {
                sqlite3_int64 val;
                getter >> val;
                return boost::any(val);
            }
            else if((*it)->def.type() == typeid(double))
            {
                double val;
                getter >> val;
                return boost::any(val);
            }
            else if((*it)->def.type() == typeid(std::string))
            {
                std::string val;
                getter >> val;
                return boost::any(val);
            }
            else if((*it)->def.type() == typeid(sql_date))
            {
                std::string val;
                getter >> val;
                return boost::any(sql_date(val));
            }
            else
            {
                // Should not happen.
                throw std::runtime_error("Field of unknown type");
            }
        }
        
        static database* db_;
        
    public:
        
        dao(database& db)
        {
            init(db);
        }
        
        template<typename V>
        static boost::shared_ptr<T> query_by__fieldname__(std::string fn, V v)
        {
            std::stringstream ss;
            
            ss << "SELECT id__, ";
            for(std::vector<sqlite::orm::base_field*>::iterator it = T::fields_.begin();
                it != T::fields_.end(); ++it)
            {
                ss << (*it)->get_name();
                if(it+1 != T::fields_.end())
                {
                    ss << ", ";
                }
            }
            ss << " " << "FROM " << T::table_name()
            << " WHERE " << fn << " = :var";
            
#ifdef DEBUG_SQL
            std::cout << ss.str() << "\n";
#endif

            query qry(*db_, ss.str().c_str());
            qry.bind(":var", v);
            
            for (sqlite3pp::query::iterator i = qry.begin(); i != qry.end(); ++i)
            {
                boost::shared_ptr<T> out = boost::shared_ptr<T>(new T);
                
                query::rows::getstream getter = (*i).getter();
                getter >> out->id__;
                
                for(std::vector<sqlite::orm::base_field*>::iterator it = T::fields_.begin();
                    it != T::fields_.end(); ++it)
                {
                    out->values_[(*it)->get_name()] = get_field(getter, it);
                }
                
                // return the first result
                return out;
            }
            
            // return empty
            return boost::shared_ptr<T>();
        }
        
        template<typename V>
        static std::vector<boost::shared_ptr<T> > query_all_by__fieldname__(std::string fn, V v)
        {
            std::stringstream ss;
            
            ss << "SELECT id__, ";
            for(std::vector<sqlite::orm::base_field*>::iterator it = T::fields_.begin();
                it != T::fields_.end(); ++it)
            {
                ss << (*it)->get_name();
                if(it+1 != T::fields_.end())
                {
                    ss << ", ";
                }
            }
            ss << " " << "FROM " << T::table_name()
            << " WHERE " << fn << " = :var";
            
#ifdef DEBUG_SQL
            std::cout << ss.str() << "\n";
#endif
            
            query qry(*db_, ss.str().c_str());
            qry.bind(":var", v);
            
            std::vector<boost::shared_ptr<T> > result;
            
            for (sqlite3pp::query::iterator i = qry.begin(); i != qry.end(); ++i)
            {
                boost::shared_ptr<T> out = boost::shared_ptr<T>(new T);
                
                query::rows::getstream getter = (*i).getter();
                getter >> out->id__;
                
                for(std::vector<sqlite::orm::base_field*>::iterator it = T::fields_.begin();
                    it != T::fields_.end(); ++it)
                {
                    out->values_[(*it)->get_name()] = get_field(getter, it);
                }
                
                result.push_back(out);
            }
            
            return result;
        }
        
        boost::shared_ptr<T> query_first()
        {
            std::string q = "SELECT id__, ";
            for(std::vector<sqlite::orm::base_field*>::iterator it = T::fields_.begin();
                it != T::fields_.end(); ++it)
            {
                q.append((*it)->get_name());
                if(it+1 != T::fields_.end())
                {
                    q.append(", ");
                }
            }
            q.append(" ").append("FROM ").append(T::table_name()).append(" LIMIT 1");

#ifdef DEBUG_SQL
            std::cout << q << "\n";
#endif
            query qry(*db_, q.c_str());
            
            for (sqlite3pp::query::iterator i = qry.begin(); i != qry.end(); ++i)
            {
                boost::shared_ptr<T> out = boost::shared_ptr<T>(new T);

                query::rows::getstream getter = (*i).getter();
                getter >> out.id__;
                
                for(std::vector<sqlite::orm::base_field*>::iterator it = T::fields_.begin();
                    it != T::fields_.end(); ++it)
                {
                    out->values_[(*it)->get_name()] = get_field(getter, it);
                }

                return out;
            }
            
            return boost::shared_ptr<T>(NULL);
        }
        
        boost::shared_ptr<T> query_first(const std::string& where, const std::map<std::string, boost::any>& args)
        {
            std::string q = "SELECT id__, ";
            for(std::vector<sqlite::orm::base_field*>::iterator it = T::fields_.begin();
                it != T::fields_.end(); ++it)
            {
                q.append((*it)->get_name());
                if(it+1 != T::fields_.end())
                {
                    q.append(", ");
                }
            }
            q.append(" ").append("FROM ").append(T::table_name()).append(" ")
                .append(where).append(" LIMIT 1");
            
#ifdef DEBUG_SQL
            std::cout << q << "\n";
#endif
            query qry(*db_, q.c_str());

            // bind all arguments
            for(std::map<std::string, boost::any>::const_iterator it = args.begin(); it != args.end(); ++it)
            {
                if(it->second.type() == typeid(long))
                {
                    qry.bind(it->first.c_str(), static_cast<long long>(boost::any_cast<long>(it->second)));
                }
                else if(it->second.type() == typeid(int))
                {
                    qry.bind(it->first.c_str(), boost::any_cast<int>(it->second));
                }
                else if(it->second.type() == typeid(sqlite3_int64))
                {
                    qry.bind(it->first.c_str(), boost::any_cast<sqlite3_int64>(it->second));
                }
                else if(it->second.type() == typeid(double))
                {
                    qry.bind(it->first.c_str(), boost::any_cast<double>(it->second));
                }
                else if(it->second.type() == typeid(std::string))
                {
                    qry.bind(it->first.c_str(), boost::any_cast<std::string>(it->second));
                }
                else if(it->second.type() == typeid(sql_date))
                {
                    qry.bind(it->first.c_str(), wrap_type(boost::any_cast<sql_date>(it->second)));
                }
                else
                {
                    // Should not happen.
                    throw std::runtime_error(std::string("Trying to bind field of unknown type: ") + it->second.type().name());
                }
            }
            
            for (sqlite3pp::query::iterator i = qry.begin(); i != qry.end(); ++i)
            {
                boost::shared_ptr<T> out = boost::shared_ptr<T>(new T);
                
                query::rows::getstream getter = (*i).getter();
                getter >> out->id__;
                
                for(std::vector<sqlite::orm::base_field*>::iterator it = T::fields_.begin();
                    it != T::fields_.end(); ++it)
                {
                    out->values_[(*it)->get_name()] = get_field(getter, it);
                }
                
                return out;
            }
            
            return boost::shared_ptr<T>();
        }
        
        std::vector<boost::shared_ptr<T> > query_all()
        {
            std::string q = "SELECT id__, ";
            for(std::vector<sqlite::orm::base_field*>::iterator it = T::fields_.begin();
                it != T::fields_.end(); ++it)
            {
                q.append((*it)->get_name());
                if(it+1 != T::fields_.end())
                {
                    q.append(", ");
                }
            }
            q.append(" ").append("FROM ").append(T::table_name());
            
#ifdef DEBUG_SQL
            std::cout << q << "\n";
#endif
            query qry(*db_, q.c_str());
            
            std::vector<boost::shared_ptr<T> > result;
            
            for (sqlite3pp::query::iterator i = qry.begin(); i != qry.end(); ++i)
            {
                boost::shared_ptr<T> out = boost::shared_ptr<T>(new T);
                
                query::rows::getstream getter = (*i).getter();
                getter >> out->id__;
                
                for(std::vector<sqlite::orm::base_field*>::iterator it = T::fields_.begin();
                    it != T::fields_.end(); ++it)
                {
                    out->values_[(*it)->get_name()] = get_field(getter, it);
                }
                
                result.push_back(out);
            }
            
            return result;
        }

        std::vector<boost::shared_ptr<T> > query_all(const std::string& where, const std::map<std::string, boost::any>& args)
        {
            std::string q = "SELECT id__, ";
            for(std::vector<sqlite::orm::base_field*>::iterator it = T::fields_.begin();
                it != T::fields_.end(); ++it)
            {
                q.append((*it)->get_name());
                if(it+1 != T::fields_.end())
                {
                    q.append(", ");
                }
            }
            q.append(" ").append("FROM ").append(T::table_name());
            
#ifdef DEBUG_SQL
            std::cout << q << "\n";
#endif
            query qry(*db_, q.c_str());
            
            // bind all arguments
            for(std::map<std::string, boost::any>::const_iterator it = args.begin(); it != args.end(); ++it)
            {
                if(it->second.type() == typeid(long))
                {
                    qry.bind(it->first.c_str(), static_cast<long long>(boost::any_cast<long>(it->second)));
                }
                else if(it->second.type() == typeid(int))
                {
                    qry.bind(it->first.c_str(), boost::any_cast<int>(it->second));
                }
                else if(it->second.type() == typeid(sqlite3_int64))
                {
                    qry.bind(it->first.c_str(), boost::any_cast<sqlite3_int64>(it->second));
                }
                else if(it->second.type() == typeid(double))
                {
                    qry.bind(it->first.c_str(), boost::any_cast<double>(it->second));
                }
                else if(it->second.type() == typeid(std::string))
                {
                    qry.bind(it->first.c_str(), boost::any_cast<std::string>(it->second));
                }
                else if(it->second.type() == typeid(sql_date))
                {
                    qry.bind(it->first.c_str(), wrap_type(boost::any_cast<sql_date>(it->second)));
                }
                else
                {
                    // Should not happen.
                    throw std::runtime_error(std::string("Trying to bind field of unknown type: ") + it->second.type().name());
                }
            }
            
            std::vector<boost::shared_ptr<T> > result;
            
            for (sqlite3pp::query::iterator i = qry.begin(); i != qry.end(); ++i)
            {
                boost::shared_ptr<T> out = boost::shared_ptr<T>(new T);
                
                query::rows::getstream getter = (*i).getter();
                getter >> out->id__;
                
                for(std::vector<sqlite::orm::base_field*>::iterator it = T::fields_.begin();
                    it != T::fields_.end(); ++it)
                {
                    out->values_[(*it)->get_name()] = get_field(getter, it);
                }
                
                result.push_back(out);
            }
            
            return result;
        }

        static void remove(boost::shared_ptr<T>& obj)
        {
            remove(*(obj.get()));
        }

        static void remove(T& obj)
        {
            // remove all cascade
            for(std::vector<base_foreign_collection*>::iterator it = T::foreign_.begin(); it != T::foreign_.end(); ++it)
            {
                char* base = (char*)&obj;
                sqlite::orm::base_foreign_collection * ptr = reinterpret_cast<sqlite::orm::base_foreign_collection*> ( base+(*it)->offset );
                ptr->remove(obj);
            }
            
            std::string q = "DELETE FROM ";
            q.append(T::table_name());
            q.append(" WHERE id__ = :var");
            
#ifdef DEBUG_SQL
            std::cout << q << "\n";
#endif
            command cmd(*db_, q.c_str());
            cmd.bind(":var", obj.id__);

#ifdef DEBUG_SQL
            std::cout << ":var == " << wrap_type(obj.id__) << "\n";
#endif
            cmd.execute();
        }
        
        static void save(T& obj)
        {
            std::string q = "INSERT OR REPLACE INTO ";
            q.append(T::table_name());
            q.append(" (id__, ");
            
            for(std::vector<sqlite::orm::base_field*>::iterator it = T::fields_.begin();
                it != T::fields_.end(); ++it)
            {
                q.append((*it)->get_name());
                if(it+1 != T::fields_.end())
                {
                    q.append(", ");
                }
            }
            
            q.append(") VALUES (");
            if(obj.id__ == -1) {
                q.append("NULL, ");
            }
            else
            {
                q.append(wrap_type(obj.id__)).append(", ");
            }
            
            for(std::vector<sqlite::orm::base_field*>::iterator it = T::fields_.begin();
                it != T::fields_.end(); ++it)
            {
                q.append(":" + (*it)->get_name());
                
                if(it+1 != T::fields_.end())
                {
                    q.append(", ");
                }
            }
            
            q.append(")");

#ifdef DEBUG_SQL
            std::cout << q << "\n";
#endif
            command cmd(*db_, q.c_str());
            
            for(std::vector<sqlite::orm::base_field*>::iterator it = T::fields_.begin();
                it != T::fields_.end(); ++it)
            {
                boost::any val = obj.values_[(*it)->get_name()];
                
                if(val.type() == typeid(long))
                {
                    cmd.bind((":" + (*it)->get_name()).c_str(), static_cast<sqlite3_int64>(boost::any_cast<long>(val)));
                }
                else if(val.type() == typeid(sqlite3_int64))
                {
                    cmd.bind((":" + (*it)->get_name()).c_str(), boost::any_cast<sqlite3_int64>(val));
                }
                else if(val.type() == typeid(double))
                {
                    cmd.bind((":" + (*it)->get_name()).c_str(), boost::any_cast<double>(val));
                }
                else if(val.type() == typeid(std::string))
                {
                    cmd.bind((":" + (*it)->get_name()).c_str(), boost::any_cast<std::string>(val).c_str());
                }
                else if(val.type() == typeid(sql_date))
                {
                    cmd.bind((":" + (*it)->get_name()).c_str(), wrap_type(boost::any_cast<sql_date>(val)));
                }
                else if(val.type() == typeid(bool))
                {
                    cmd.bind((":" + (*it)->get_name()).c_str(), boost::any_cast<bool>(val));
                }
                else
                {
                    cmd.bind((":" + (*it)->get_name()).c_str(), boost::any_cast<int>(val));
                }
            }
            
            cmd.execute();
            
            // Set new id
            if(obj.id__ == -1) {
                obj.id__ = db_->last_insert_rowid();
            }
            
            // now when we have the id__ lets save the foreign collections if we have any
            for(std::vector<base_foreign_collection*>::iterator it = T::foreign_.begin(); it != T::foreign_.end(); ++it)
            {
                char* base = (char*)&obj;
                sqlite::orm::base_foreign_collection * ptr = reinterpret_cast<sqlite::orm::base_foreign_collection*> ( base+(*it)->offset );
                ptr->save(obj);
            }
        }
    };
    
    // Linker happy-happy ;)
    template<typename T>
    database* dao<T>::db_;
    
    struct base_model
    {
    };
    
    template<typename T>
    class model
    : public base_model
    {
    public:
        friend class sqlite::orm::dao<T>;
        
        model()
        : id__(-1)
        {
            for(std::vector<sqlite::orm::base_field*>::iterator it = T::fields_.begin();
                it != T::fields_.end(); ++it)
            {
                // init with default value
                values_[(*it)->get_name()] = (*it)->def;
            }        
        }
        
        static const std::string table_name()
        {
            return typeid(T).name();
        }
        
        static void add_field(sqlite::orm::base_field* f, int offset)
        {
            fields_.push_back(f);
        }

        static void add_foreign_collection(sqlite::orm::base_foreign_collection* f, int offset)
        {
            f->offset = offset;
            foreign_.push_back(f);
        }
        
        static std::vector<sqlite::orm::base_field*> fields_;
        static std::vector<sqlite::orm::base_foreign_collection*> foreign_;
        
        bool operator ==(const model& m)
        {
            return id__ == m.id__;
        }
        
        bool operator ==(const sqlite3_int64& i)
        {
            return id__ == i;
        }
        
        const sqlite3_int64 get_id() const
        {
            return id__;
        }
        
    protected:
        sqlite3_int64 id__;
        std::map<std::string, boost::any> values_;
    };
    
    template<typename T>
    std::vector<sqlite::orm::base_field*> model<T>::fields_;

    template<typename T>
    std::vector<sqlite::orm::base_foreign_collection*> model<T>::foreign_;
    
#define STR(a) #a
    
/*
 *  COMMON PART
 */
#define STD_FIELD_BODY(name, type)\
: public sqlite::orm::base_field \
{\
void init()\
{\
static bool _init = false; \
if(!_init)\
{\
sqlite::orm::model<MODEL_NAME>::add_field(this, offsetof(MODEL_NAME, name));\
_init = true; \
}\
}\
\
virtual const std::string get_name()\
{\
return #name;\
}\
virtual const std::string get_type()\
{\
return type;\
}\

/*
 *  TEXT
 */
#define FIELD_STR(name) \
struct field_##name \
STD_FIELD_BODY(name, "TEXT") \
field_##name(const std::string& def)\
: sqlite::orm::base_field(def)\
{\
init(); \
}\
};\
private:\
field_##name name;\
public:\
boost::shared_ptr<MODEL_NAME> query_by_##name(const std::string& v) \
{\
return sqlite::orm::dao<MODEL_NAME>::query_by__fieldname__(#name, v);\
}\
std::vector<boost::shared_ptr<MODEL_NAME> > query_all_by_##name(const std::string& v) \
{\
return sqlite::orm::dao<MODEL_NAME>::query_all_by__fieldname__(#name, v);\
}\
const std::string get_##name() const { return boost::any_cast<std::string>(values_.at(#name)); } \
void set_##name(const std::string& s) { values_[#name] = s; }

/*
 *  DATE
 */
#define FIELD_DATE(name) \
struct field_##name \
STD_FIELD_BODY(name, "DATE") \
field_##name(const sqlite::orm::sql_date& def)\
: sqlite::orm::base_field(def)\
{\
init(); \
}\
};\
private:\
field_##name name;\
public:\
boost::shared_ptr<MODEL_NAME> query_by_##name(const sqlite::orm::sql_date& v) \
{\
return sqlite::orm::dao<MODEL_NAME>::query_by__fieldname__(#name, v);\
}\
std::vector<boost::shared_ptr<MODEL_NAME> > query_all_by_##name(const sqlite::orm::sql_date& v) \
{\
return sqlite::orm::dao<MODEL_NAME>::query_all_by__fieldname__(#name, v);\
}\
const sqlite::orm::sql_date get_##name() const { return boost::any_cast<sqlite::orm::sql_date>(values_.at(#name)); } \
void set_##name(const sqlite::orm::sql_date& d) { values_[#name] = d; }
    
/*
 *  NUMBER
 */
#define FIELD_NUM(name) \
struct field_##name \
STD_FIELD_BODY(name, "NUM")\
field_##name(long def)\
: sqlite::orm::base_field((long)def)\
{\
init(); \
}\
};\
private:\
field_##name name;\
public:\
boost::shared_ptr<MODEL_NAME> query_by_##name(long v) \
{\
return sqlite::orm::dao<MODEL_NAME>::query_by__fieldname__(#name, v);\
}\
std::vector<boost::shared_ptr<MODEL_NAME> > query_all_by_##name(long v) \
{\
return sqlite::orm::dao<MODEL_NAME>::query_all_by__fieldname__(#name, v);\
}\
const long get_##name() const { return boost::any_cast<long>(values_.at(#name)); } \
void set_##name(const long& l) { values_[#name] = l; }

/*
 *  INTEGER
 */
#define FIELD_INT(name) \
struct field_##name \
STD_FIELD_BODY(name, "INTEGER")\
field_##name(int def)\
: sqlite::orm::base_field((int)def)\
{\
init(); \
}\
};\
private:\
field_##name name;\
public:\
boost::shared_ptr<MODEL_NAME> query_by_##name(int v) \
{\
return sqlite::orm::dao<MODEL_NAME>::query_by__fieldname__(#name, v);\
}\
std::vector<boost::shared_ptr<MODEL_NAME> > query_all_by_##name(int v) \
{\
return sqlite::orm::dao<MODEL_NAME>::query_all_by__fieldname__(#name, v);\
}\
const int get_##name() const { return boost::any_cast<int>(values_.at(#name)); } \
void set_##name(const int& i) { values_[#name] = i; }

/*
 *  INTEGER 64
 */
#define FIELD_INT64(name) \
struct field_##name \
STD_FIELD_BODY(name, "INTEGER")\
field_##name(int64_t def)\
: sqlite::orm::base_field((int64_t)def)\
{\
init(); \
}\
};\
private:\
field_##name name;\
public:\
boost::shared_ptr<MODEL_NAME> query_by_##name(int64_t v) \
{\
return sqlite::orm::dao<MODEL_NAME>::query_by__fieldname__(#name, v);\
}\
std::vector<boost::shared_ptr<MODEL_NAME> > query_all_by_##name(int64_t v) \
{\
return sqlite::orm::dao<MODEL_NAME>::query_all_by__fieldname__(#name, v);\
}\
const int get_##name() const { return boost::any_cast<int64_t>(values_.at(#name)); } \
void set_##name(const int64_t& i) { values_[#name] = i; }

    
/*
 *  BOOLEAN
 */
#define FIELD_BOOL(name) \
struct field_##name \
STD_FIELD_BODY(name, "INTEGER")\
field_##name(bool def)\
: sqlite::orm::base_field((int)(def?1:0))\
{\
init(); \
}\
};\
private:\
field_##name name;\
public:\
boost::shared_ptr<MODEL_NAME> query_by_##name(bool v) \
{\
return sqlite::orm::dao<MODEL_NAME>::query_by__fieldname__(#name, v?1:0);\
}\
std::vector<boost::shared_ptr<MODEL_NAME> > query_all_by_##name(bool v) \
{\
return sqlite::orm::dao<MODEL_NAME>::query_all_by__fieldname__(#name, v);\
}\
const bool get_##name() const { return boost::any_cast<bool>(values_.at(#name)); } \
void set_##name(const bool& b) { values_[#name] = b; }

/*
 *  REAL
 */
#define FIELD_REAL(name) \
struct field_##name \
STD_FIELD_BODY(name, "REAL")\
field_##name(double def)\
: sqlite::orm::base_field((double)def)\
{\
init(); \
}\
};\
private:\
field_##name name;\
public:\
boost::shared_ptr<MODEL_NAME> query_by_##name(double v) \
{\
return sqlite::orm::dao<MODEL_NAME>::query_by__fieldname__(#name, v);\
}\
std::vector<boost::shared_ptr<MODEL_NAME> > query_all_by_##name(double v) \
{\
return sqlite::orm::dao<MODEL_NAME>::query_all_by__fieldname__(#name, v);\
}\
const double get_##name() const { return boost::any_cast<double>(values_.at(#name)); } \
void set_##name(const double& d) { values_[#name] = d; }

/*
 *  HAS MANY IMPL (FOREIGN COLLECTION)
 *  Used together with BELONGS_TO to fetch a collection of
 *  foreign objects on demand (lazy).
 */
#define HAS_MANY_IMPL(model, cls, name) \
void model::save_foreign_##name(model& obj) \
{ \
    for(std::vector<cls>::iterator it = obj.name##_.collection.begin(); it != obj.name##_.collection.end(); ++it) \
    { \
        it->values_[STR(model##_id)] = obj.id__; \
        sqlite::orm::dao<cls>::save(*it); \
    } \
} \
void model::remove_foreign_##name(model& obj) \
{ \
    std::vector<boost::shared_ptr<cls> > col = obj.fetch_##name(); \
    for(std::vector<boost::shared_ptr<cls> >::iterator it = col.begin(); it != col.end(); ++it) \
    { \
        sqlite::orm::dao<cls>::remove(*it); \
    } \
} \
std::vector<boost::shared_ptr<cls> > model::fetch_##name() \
{ \
    std::vector<boost::shared_ptr<cls> > tmp = sqlite::orm::dao<cls>::query_by_##model(*this);\
    name##_.collection.clear();\
    for(int i=0; i<tmp.size(); ++i)\
    {\
        name##_.collection.push_back(*tmp.at(i));\
    }\
    return tmp;\
} \
void model::add_to_##name(const cls& i) \
{ \
    name##_.collection.push_back(i); \
} \
void model::clear_##name() \
{ \
    name##_.collection.clear(); \
} \
void model::remove_from_##name(const cls& i) \
{ \
    name##_.collection.erase(std::remove(name##_.collection.begin(), \
        name##_.collection.end(), i), name##_.collection.end()); \
} \
std::vector<cls>& model::get_##name() \
{ \
    return name##_.collection; \
} \
void model::set_##name(const std::vector<cls>& v) { name##_.collection = v; }


/*
 *  HAS MANY (FOREIGN COLLECTION)
 *  Used together with BELONGS_TO to fetch a collection of
 *  foreign objects on demand (lazy).
 */    
#define HAS_MANY(cls, name) \
friend class cls; \
private: \
    static void save_foreign_##name(MODEL_NAME& obj); \
    static void remove_foreign_##name(MODEL_NAME& obj); \
public: \
struct foreign_collection_##cls \
: sqlite::orm::foreign_collection<cls> \
{ \
    void init()\
    {\
        static bool _init = false; \
        if(!_init)\
        {\
            sqlite::orm::model<MODEL_NAME>::add_foreign_collection(this, offsetof(MODEL_NAME, name##_));\
            _init = true; \
        }\
    }\
    foreign_collection_##cls() \
    { \
        init(); \
    } \
    void save(base_model& obj) \
    { \
        MODEL_NAME::save_foreign_##name(*static_cast<MODEL_NAME*>(&obj)); \
    } \
    void remove(base_model& obj) \
    { \
        MODEL_NAME::remove_foreign_##name(*static_cast<MODEL_NAME*>(&obj)); \
    } \
}; \
private: \
    foreign_collection_##cls name##_; \
public: \
std::vector<boost::shared_ptr<cls> > fetch_##name(); \
void add_to_##name(const cls& i); \
void clear_##name(); \
void remove_from_##name(const cls& i); \
std::vector<cls>& get_##name(); \
void set_##name(const std::vector<cls>& v);
    
/*
 *  BELONGS TO
 *  Just makes a simple field which acts as foreign key
 */
#define BELONGS_TO(cls, name) \
friend class cls; \
struct belongs_to_##cls \
STD_FIELD_BODY(cls##_id, "INTEGER")\
belongs_to_##cls(cls& v); \
belongs_to_##cls()\
: sqlite::orm::base_field((sqlite3_int64)-1)\
{\
init(); \
}\
};\
private:\
belongs_to_##cls cls##_id;\
public:\
static std::vector<boost::shared_ptr<MODEL_NAME> > query_by_##cls(cls& v); \
boost::shared_ptr<cls> get_##name() const;

/*
 *  BELONGS TO IMPL
 */
#define BELONGS_TO_IMPL(model, cls, name) \
model::belongs_to_##cls::belongs_to_##cls(cls& v)\
: sqlite::orm::base_field((sqlite3_int64)(v.id__))\
{\
init(); \
}\
std::vector<boost::shared_ptr<model> > model::query_by_##cls(cls& v) \
{\
return sqlite::orm::dao<model>::query_all_by__fieldname__(STR(cls##_id), v.id__); \
}\
boost::shared_ptr<cls> model::get_##name() const \
{\
return sqlite::orm::dao<cls>::query_by__fieldname__("id__", boost::any_cast<sqlite3_int64>(values_.at(STR(cls##_id))));\
}\
    
/*
 *  MODEL DECLARATION
 */
#define BEGIN_MODEL_DECLARATION() \
class MODEL_NAME \
: public sqlite::orm::model<MODEL_NAME> \
{ \
public:    \
MODEL_NAME()

#define END_DECLARATION() \
};
    
}; // orm
}; // sqlite

#endif // _SQLITE_ORM_H_
