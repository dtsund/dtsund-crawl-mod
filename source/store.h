/**
 * @file
 * @brief Saveable hash-table and vector capable of storing
 *             multiple types of data.
**/

#ifndef STORE_H
#define STORE_H

#include <limits.h>
#include <map>
#include <string>
#include <vector>

class  reader;
class  writer;
class  CrawlHashTable;
class  CrawlVector;
struct item_def;
struct coord_def;
struct level_pos;
class  level_id;
class  dlua_chunk;
class monster;

typedef uint8_t hash_size;
typedef uint8_t vec_size;
typedef uint8_t store_flags;

#define VEC_MAX_SIZE  255
#define HASH_MAX_SIZE 255

// NOTE: Changing the ordering of these enums will break savefile
// compatibility.
enum store_val_type
{
    SV_NONE = 0,
    SV_BOOL,
    SV_BYTE,
    SV_SHORT,
    SV_INT,
    SV_FLOAT,
    SV_STR,
    SV_COORD,
    SV_ITEM,
    SV_HASH,
    SV_VEC,
    SV_LEV_ID,
    SV_LEV_POS,
    SV_MONST,
    SV_LUA,
    SV_INT64,
    NUM_STORE_VAL_TYPES
};

enum store_flag_type
{
    SFLAG_UNSET      = (1 << 0),
    SFLAG_CONST_VAL  = (1 << 1),
    SFLAG_CONST_TYPE = (1 << 2),
    SFLAG_NO_ERASE   = (1 << 3),
};


// Can't just cast everything into a void pointer, since a float might
// not fit into a pointer on all systems.
typedef union StoreUnion StoreUnion;
union StoreUnion
{
    bool  boolean;
    char  byte;
    short _short;
    int   _int;
    float _float;
    int64_t _int64;
    void* ptr;
};


class CrawlStoreValue
{
public:
    CrawlStoreValue();
    CrawlStoreValue(const CrawlStoreValue &other);

    ~CrawlStoreValue();

    // Conversion constructors
    CrawlStoreValue(const bool val);
    CrawlStoreValue(const char &val);
    CrawlStoreValue(const short &val);
    CrawlStoreValue(const int &val);
    CrawlStoreValue(const int64_t &val);
    CrawlStoreValue(const float &val);
    CrawlStoreValue(const std::string &val);
    CrawlStoreValue(const char* val);
    CrawlStoreValue(const coord_def &val);
    CrawlStoreValue(const item_def &val);
    CrawlStoreValue(const CrawlHashTable &val);
    CrawlStoreValue(const CrawlVector &val);
    CrawlStoreValue(const level_id &val);
    CrawlStoreValue(const level_pos &val);
    CrawlStoreValue(const monster& val);
    CrawlStoreValue(const dlua_chunk &val);

    CrawlStoreValue &operator = (const CrawlStoreValue &other);

protected:
    store_val_type type;
    store_flags    flags;
    StoreUnion     val;

public:
    store_flags    get_flags() const;
    store_flags    set_flags(store_flags flags);
    store_flags    unset_flags(store_flags flags);
    store_val_type get_type() const;

    CrawlHashTable &new_table();

    CrawlVector &new_vector(store_flags flags,
                            vec_size max_size = VEC_MAX_SIZE);
    CrawlVector &new_vector(store_val_type type, store_flags flags = 0,
                            vec_size max_size = VEC_MAX_SIZE);

    bool           &get_bool();
    char           &get_byte();
    short          &get_short();
    int            &get_int();
    int64_t        &get_int64();
    float          &get_float();
    std::string    &get_string();
    coord_def      &get_coord();
    CrawlHashTable &get_table();
    CrawlVector    &get_vector();
    item_def       &get_item();
    level_id       &get_level_id();
    level_pos      &get_level_pos();
    monster        &get_monster();
    dlua_chunk     &get_lua();

    bool           get_bool()      const;
    char           get_byte()      const;
    short          get_short()     const;
    int            get_int()       const;
    int64_t        get_int64()     const;
    float          get_float()     const;
    std::string    get_string()    const;
    coord_def      get_coord()     const;
    level_id       get_level_id()  const;
    level_pos      get_level_pos() const;

    const CrawlHashTable& get_table()  const;
    const CrawlVector&    get_vector() const;
    const item_def&       get_item()   const;
    const monster&       get_monster() const;
    const dlua_chunk&     get_lua() const;

public:
    // NOTE: All operators will assert if the value is of the wrong
    // type for the operation.  If the value has no type yet, the
    // operation will set it to the appropriate type.  If the value
    // has no type yet and the operation modifies the existing value
    // rather than replacing it (e.g., ++) the value will be set to a
    // default before the operation is done.

    // If the value is a hash table or vector, the container's values
    // can be accessed with the [] operator with the approriate key
    // type (strings for hashes, longs for vectors).
    CrawlStoreValue &operator [] (const std::string   &key);
    CrawlStoreValue &operator [] (const vec_size &index);

    const CrawlStoreValue &operator [] (const std::string &key) const;
    const CrawlStoreValue &operator [] (const vec_size &index)  const;

    // Typecast operators
    operator bool&();
    operator char&();
    operator short&();
    operator int&();
    operator int64_t&();
    operator float&();
    operator std::string&();
    operator coord_def&();
    operator CrawlHashTable&();
    operator CrawlVector&();
    operator item_def&();
    operator level_id&();
    operator level_pos&();
    operator monster& ();
    operator dlua_chunk&();

    operator bool()        const;
    operator char()        const;
    operator short()       const;
    operator int()         const;
    operator int64_t()     const;
    operator float()       const;
    operator std::string() const;
    operator coord_def()   const;
    operator level_id()    const;
    operator level_pos()   const;

    // Assignment operators
    CrawlStoreValue &operator = (const bool &val);
    CrawlStoreValue &operator = (const char &val);
    CrawlStoreValue &operator = (const short &val);
    CrawlStoreValue &operator = (const int &val);
    CrawlStoreValue &operator = (const int64_t &val);
    CrawlStoreValue &operator = (const float &val);
    CrawlStoreValue &operator = (const std::string &val);
    CrawlStoreValue &operator = (const char* val);
    CrawlStoreValue &operator = (const coord_def &val);
    CrawlStoreValue &operator = (const CrawlHashTable &val);
    CrawlStoreValue &operator = (const CrawlVector &val);
    CrawlStoreValue &operator = (const item_def &val);
    CrawlStoreValue &operator = (const level_id &val);
    CrawlStoreValue &operator = (const level_pos &val);
    CrawlStoreValue &operator = (const monster& val);
    CrawlStoreValue &operator = (const dlua_chunk &val);

    // Misc operators
    std::string &operator += (const std::string &val);

    // Prefix
    int operator ++ ();
    int operator -- ();

    // Postfix
    int operator ++ (int);
    int operator -- (int);

protected:
    CrawlStoreValue(const store_flags flags,
                    const store_val_type type = SV_NONE);

    void write(writer &) const;
    void read(reader &);

    void unset(bool force = false);

    friend class CrawlHashTable;
    friend class CrawlVector;
};


// A hash table can have a maximum of 255 key/value pairs.  If you
// want more than that you can use nested hash tables.
//
// By default a hash table's value data types are heterogeneous.  To
// make it homogeneous (which causes dynamic type checking) you have
// to give a type to the hash table constructor; once it's been
// created its type (or lack of type) is immutable.
//
// An empty hash table will take up only 1 byte in the savefile.  A
// non-empty hash table will have an overhead of 3 bytes for the hash
// table overall and 2 bytes per key/value pair, over and above the
// number of bytes needed to store the keys and values themselves.
class CrawlHashTable
{
public:
    CrawlHashTable();
    CrawlHashTable(const CrawlHashTable& other);

    ~CrawlHashTable();

    typedef std::map<std::string, CrawlStoreValue> hash_map_type;
    typedef hash_map_type::iterator                iterator;
    typedef hash_map_type::const_iterator          const_iterator;

protected:
    // NOTE: Not using std::auto_ptr because making hash_map an auto_ptr
    // causes compile weirdness in externs.h
    hash_map_type *hash_map;

    void init_hash_map();

    friend class CrawlStoreValue;

public:
    CrawlHashTable &operator = (const CrawlHashTable &other);

    void write(writer &) const;
    void read(reader &);

    bool exists(const std::string &key) const;
    void assert_validity() const;

    // NOTE: If the const versions of get_value() or [] are given a
    // key which doesn't exist, they will assert.
    const CrawlStoreValue& get_value(const std::string &key) const;
    const CrawlStoreValue& operator[] (const std::string &key) const;

    // NOTE: If get_value() or [] is given a key which doesn't exist
    // in the table, an unset/empty CrawlStoreValue will be created
    // with that key and returned.  If it is not then given a value
    // then the next call to assert_validity() will fail.  If the
    // hash table has a type (rather than being heterogeneous)
    // then trying to assign a different type to the CrawlStoreValue
    // will assert.
    CrawlStoreValue& get_value(const std::string &key);
    CrawlStoreValue& operator[] (const std::string &key);

    // std::map style interface
    hash_size size() const;
    bool      empty() const;

    void      erase(const std::string key);
    void      clear();

    const_iterator begin() const;
    const_iterator end() const;

    iterator  begin();
    iterator  end();
};

// A CrawlVector is the vector version of CrawlHashTable, except that
// a non-empty CrawlVector has one more byte of savefile overhead that
// a hash table, and that can specify a maximum size to make it act
// similarly to a FixedVec.
class CrawlVector
{
public:
    CrawlVector();
    CrawlVector(store_flags flags, vec_size max_size = VEC_MAX_SIZE);
    CrawlVector(store_val_type type, store_flags flags = 0,
                vec_size max_size = VEC_MAX_SIZE);

    ~CrawlVector();

    typedef std::vector<CrawlStoreValue> vector_type;
    typedef vector_type::iterator        iterator;
    typedef vector_type::const_iterator  const_iterator;

protected:
    store_val_type type;
    store_flags    default_flags;
    vec_size       max_size;
    vector_type    vector;

    friend class CrawlStoreValue;

public:
    void write(writer &) const;
    void read(reader &);

    store_flags    get_default_flags() const;
    store_flags    set_default_flags(store_flags flags);
    store_flags    unset_default_flags(store_flags flags);
    store_val_type get_type() const;
    void           assert_validity() const;
    void           set_max_size(vec_size size);
    vec_size       get_max_size() const;

    // NOTE: If the const versions of get_value() or [] are given an
    // index which doesn't exist, they will assert.
    const CrawlStoreValue& get_value(const vec_size &index) const;
    const CrawlStoreValue& operator[] (const vec_size &index) const;

    CrawlStoreValue& get_value(const vec_size &index);
    CrawlStoreValue& operator[] (const vec_size &index);

    // std::vector style interface
    vec_size size() const;
    bool     empty() const;

    // NOTE: push_back() and insert() have val passed by value rather
    // than by reference so that conversion constructors will work.
    CrawlStoreValue& pop_back();
    void             push_back(CrawlStoreValue val);
    void insert(const vec_size index, CrawlStoreValue val);

    // resize() will assert if the maximum size has been set.
    void resize(const vec_size size);
    void erase(const vec_size index);
    void clear();

    const_iterator begin() const;
    const_iterator end() const;

    iterator begin();
    iterator end();
};
#endif
