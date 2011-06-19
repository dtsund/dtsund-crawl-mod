/**
 * @file
 * @brief dbm wrapper for SQLite
**/

#include "AppHdr.h"

#include "libutil.h"
#include "sqldbm.h"
#include "stuff.h"
#include <fcntl.h>
#include <cstring>

#ifdef USE_SQLITE_DBM

class sqlite_retry_iterator
{
public:
    sqlite_retry_iterator(int _nretries = 50)
        : nretries(_nretries)
    {
    }
    operator bool () const
    {
        return nretries > 0;
    }
    void check(int errcode)
    {
        if (errcode == SQLITE_BUSY)
        {
            --nretries;
            // Give the annoying process locking the db a little time
            // to finish whatever it's up to before we retry.
            usleep(1000);
        }
        else
        {
            nretries = 0;
        }
    }
private:
    int nretries;
};

SQL_DBM::SQL_DBM(const std::string &dbname, bool _readonly, bool do_open)
    : error(), errc(SQLITE_OK), db(NULL), s_insert(NULL), s_remove(NULL),
      s_query(NULL), s_iterator(NULL), dbfile(dbname), readonly(_readonly)
{
    if (do_open && !dbfile.empty())
        open();
}

SQL_DBM::~SQL_DBM()
{
    close();
}

int SQL_DBM::ec(int err)
{
    if (err == SQLITE_OK)
        error.clear();
    else if (db)
        error = sqlite3_errmsg(db);
    else
        error = "Unknown error";

    return (errc = err);
}

bool SQL_DBM::is_open() const
{
    return !!db;
}

int SQL_DBM::open(const std::string &s)
{
    close();

    if (!s.empty())
        dbfile = s;

    if (dbfile.empty())
    {
        error = "No filename!";
        return SQLITE_ERROR; // "... or missing database"
    }

    if (dbfile.find(".db") != dbfile.length() - 3)
        dbfile += ".db";

/*
From SQLite's documentation:

# Note to Windows users: The encoding used for the filename argument of
# sqlite3_open() and sqlite3_open_v2() must be UTF-8, not whatever codepage
# is currently defined.  Filenames containing international characters must
# be converted to UTF-8 prior to passing them into sqlite3_open() or
# sqlite3_open_v2().

... which saves us a lot of trouble.
*/
#ifdef ANCIENT_SQLITE
    if (ec(sqlite3_open(
                dbfile.c_str(), &db
#else
    if (ec(sqlite3_open_v2(
                dbfile.c_str(), &db,
                readonly? SQLITE_OPEN_READONLY :
                (SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE),
                NULL
#endif
              )) != SQLITE_OK)
    {
        const std::string saveerr = error;
        const int serrc = errc;
        close();
        error = saveerr;
        errc  = serrc;
        return (errc);
    }

    init_schema();
    return (errc);
}

int SQL_DBM::init_schema()
{
    int err = ec(sqlite3_exec(
                  db,
                  "CREATE TABLE dbm (key STRING UNIQUE PRIMARY KEY,"
                  "                  value STRING);",
                  NULL,
                  NULL,
                  NULL));

    // Turn off auto-commit
    if (!readonly)
    {
        for (sqlite_retry_iterator ri; ri;
             ri.check(ec(sqlite3_exec(db, "BEGIN;", NULL, NULL, NULL))));
    }
    return (err);
}

void SQL_DBM::close()
{
    if (db)
    {
        if (!readonly)
            sqlite3_exec(db, "COMMIT;", NULL, NULL, NULL);
        finalise_query(&s_insert);
        finalise_query(&s_remove);
        finalise_query(&s_query);
        finalise_query(&s_iterator);
        sqlite3_close(db);
        db = NULL;
    }
}

int SQL_DBM::try_insert(const std::string &key, const std::string &value)
{
    if (init_insert() != SQLITE_OK)
        return (errc);

    ec(sqlite3_bind_text(s_insert, 1, key.c_str(), -1, SQLITE_TRANSIENT));
    if (errc != SQLITE_OK)
        return (errc);
    ec(sqlite3_bind_text(s_insert, 2, value.c_str(), -1, SQLITE_TRANSIENT));
    if (errc != SQLITE_OK)
        return (errc);

    ec(sqlite3_step(s_insert));
    sqlite3_reset(s_insert);

    return (errc);
}

int SQL_DBM::insert(const std::string &key, const std::string &value)
{
    for (sqlite_retry_iterator ri; ri;
         ri.check(do_insert(key, value)));
    return (errc);
}

int SQL_DBM::do_insert(const std::string &key, const std::string &value)
{
    try_insert(key, value);
    if (errc != SQLITE_OK)
    {
        remove(key);
        try_insert(key, value);
    }
    return (errc);
}

int SQL_DBM::init_insert()
{
    return s_insert? SQLITE_OK :
        prepare_query(&s_insert, "INSERT INTO dbm VALUES (?, ?)");
}

int SQL_DBM::remove(const std::string &key)
{
    if (init_remove() != SQLITE_OK)
        return (errc);

    ec(sqlite3_bind_text(s_remove, 1, key.c_str(), -1, SQLITE_TRANSIENT));
    if (errc != SQLITE_OK)
        return (errc);

    ec(sqlite3_step(s_remove));
    sqlite3_reset(s_remove);

    return (errc);
}

int SQL_DBM::init_remove()
{
    return s_remove? SQLITE_OK :
        prepare_query(&s_remove, "DELETE FROM dbm WHERE key = ?");
}

int SQL_DBM::do_query(const std::string &key, std::string *result)
{
    if (init_query() != SQLITE_OK)
        return (errc);

    if (ec(sqlite3_bind_text(s_query, 1, key.c_str(), -1, SQLITE_TRANSIENT))
        != SQLITE_OK)
    {
        return (errc);
    }

    int err = SQLITE_OK;
    std::string res;
    while ((err = ec(sqlite3_step(s_query))) == SQLITE_ROW)
        *result = (const char *) sqlite3_column_text(s_query, 0);

    sqlite3_reset(s_query);

    if (err == SQLITE_DONE)
        err = SQLITE_OK;

    return (ec(err));
}

std::string SQL_DBM::query(const std::string &key)
{
    std::string result;
    for (sqlite_retry_iterator ri; ri;
         ri.check(do_query(key, &result)));
    return (result);
}

std::auto_ptr<std::string> SQL_DBM::firstkey()
{
    if (init_iterator() != SQLITE_OK)
    {
        std::auto_ptr<std::string> result;
        return (result);
    }

    return nextkey();
}

std::auto_ptr<std::string> SQL_DBM::nextkey()
{
    std::auto_ptr<std::string> result;
    if (s_iterator)
    {
        int err = SQLITE_OK;
        if ((err = ec(sqlite3_step(s_iterator))) == SQLITE_ROW)
            result.reset(
                new std::string(
                    (const char *) sqlite3_column_text(s_iterator, 0)));
        else
            sqlite3_reset(s_iterator);
    }
    return (result);
}

int SQL_DBM::init_query()
{
    return s_query? SQLITE_OK :
        prepare_query(&s_query, "SELECT value FROM dbm WHERE key = ?");
}

int SQL_DBM::init_iterator()
{
    return s_iterator? SQLITE_OK :
        prepare_query(&s_iterator, "SELECT key FROM dbm");
}

int SQL_DBM::finalise_query(sqlite3_stmt **q)
{
    if (!*q)
        return (SQLITE_OK);

    sqlite3_reset(*q);
    int ret = ec(sqlite3_finalize(*q));
    *q = NULL;

    return (ret);
}

int SQL_DBM::prepare_query(sqlite3_stmt **q, const char *sql)
{
    if (*q)
        finalise_query(q);

    const char *query_tail;
#ifdef ANCIENT_SQLITE
    return ec(sqlite3_prepare(db, sql, -1, q, &query_tail));
#else
    return ec(sqlite3_prepare_v2(db, sql, -1, q, &query_tail));
#endif
}

////////////////////////////////////////////////////////////////////////

sql_datum::sql_datum() : dptr(NULL), dsize(0), need_free(false)
{
}

sql_datum::sql_datum(const std::string &s) : dptr(NULL), dsize(s.length()),
                                             need_free(false)
{
    if ((dptr = new char [dsize]))
    {
        if (dsize)
            memcpy(dptr, s.c_str(), dsize);
        need_free = true;
    }
}

sql_datum::sql_datum(const sql_datum &dat) : dptr(NULL), dsize(0), need_free(false)
{
    init_from(dat);
}

sql_datum::~sql_datum()
{
    reset();
}

sql_datum &sql_datum::operator = (const sql_datum &d)
{
    if (&d != this)
    {
        reset();
        init_from(d);
    }
    return (*this);
}

void sql_datum::reset()
{
    if (need_free)
        delete [] dptr;

    dptr = NULL;
    dsize = 0;
}

void sql_datum::init_from(const sql_datum &d)
{
    dsize = d.dsize;
    need_free = false;
    if (d.need_free)
    {
        if ((dptr = new char [dsize]))
        {
            if (dsize)
                memcpy(dptr, d.dptr, dsize);
            need_free = true;
        }
    }
    else
    {
        need_free = false;
        dptr      = d.dptr;
    }
}

std::string sql_datum::to_str() const
{
    return std::string(dptr, dsize);
}

////////////////////////////////////////////////////////////////////////

SQL_DBM *dbm_open(const char *filename, int mode, int)
{
    SQL_DBM *n = new SQL_DBM(filename, mode == O_RDONLY, true);
    if (!n->is_open())
    {
        delete n;
        return (NULL);
    }

    return (n);
}

int dbm_close(SQL_DBM *db)
{
    delete db;
    return (0);
}

sql_datum dbm_fetch(SQL_DBM *db, const sql_datum &key)
{
    std::string ans = db->query(std::string(key.dptr, key.dsize));
    return sql_datum(ans);
}

static sql_datum dbm_key(
    SQL_DBM *db,
    std::auto_ptr<std::string> (SQL_DBM::*key)())
{
    std::auto_ptr<std::string> res = (db->*key)();
    if (res.get())
        return sql_datum(*res.get());
    else
    {
        sql_datum dummy;
        return dummy;
    }
}

sql_datum dbm_firstkey(SQL_DBM *db)
{
    return dbm_key(db, &SQL_DBM::firstkey);
}

sql_datum dbm_nextkey(SQL_DBM *db)
{
    return dbm_key(db, &SQL_DBM::nextkey);
}

int dbm_store(SQL_DBM *db, const sql_datum &key, const sql_datum &value, int)
{
    int err = db->insert(key.to_str(), value.to_str());
    if (err == SQLITE_DONE || err == SQLITE_CONSTRAINT)
        err = SQLITE_OK;
    else
        end(1, false, "%d: %s", db->errc, db->error.c_str());
    return (err);
}

#endif // USE_SQLITE_DBM
