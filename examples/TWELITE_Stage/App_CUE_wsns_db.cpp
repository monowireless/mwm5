/* Copyright (C) 2022 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include "App_CUE.hpp"

#include <memory>
#include <functional>
#include <optional>
#include <cmath>
#include <algorithm>

#define WSNS_DB_FILENAME "_WSns.sqlite" // DB file suffix.

#define WSNS_EXPORT_FILENAME "WSns_"
#define WSNS_EXPORT_FILEEXT "csv"

#define WSNS_DB_COMMIT_PERIOD 10 // commit priod

#define PKT_TYPE_ARIA uint8_t(E_PAL_DATA_TYPE::EX_ARIA_STD)
#define PKT_TYPE_AMB uint8_t(E_PAL_DATA_TYPE::AMB_STD)
#define PKT_TYPE_MOT uint8_t(E_PAL_DATA_TYPE::MOT_STD)
#define PKT_TYPE_CUE uint8_t(E_PAL_DATA_TYPE::EX_CUE_STD)
#define PKT_TYPE_MAG uint8_t(E_PAL_DATA_TYPE::MAG_STD)
#define PKT_TYPE_APPTWELITE (0x100 + (uint16_t)E_PKT::PKT_TWELITE)
#define PKT_TYPE_APPIO (0x100 + (uint16_t)E_PKT::PKT_APPIO)
//#define IS_PKT_TYPE_PAL(x) (x && x <= 0xFF)
#define IS_PKT_TYPE_ARIA_AMB(x) (x == uint8_t(E_PAL_DATA_TYPE::AMB_STD) || x == uint8_t(E_PAL_DATA_TYPE::EX_ARIA_STD))
#define IS_PKT_TYPE_MOT_CUE(x) (x == uint8_t(E_PAL_DATA_TYPE::MOT_STD) || x == uint8_t(E_PAL_DATA_TYPE::EX_CUE_STD))
#define IS_PKT_APPTWELITE(x) (x == PKT_TYPE_APPTWELITE)
#define IS_PKT_APPIO(x) (x == PKT_TYPE_APPIO)
#define IS_PKT_MAG(x) (x == PKT_TYPE_MAG)

//#define WSNS_DEBUG_FUNCTIONS
//#define WSNS_DB_USE_YMDH

#ifdef WSNS_DEBUG_FUNCTIONS
// RANDOM GENERATOR
#include <random>
#endif

// show as busy
extern void screen_set_busy();
extern void screen_unset_busy();

extern void screen_hide_cursor();
extern void screen_show_cursor();
class SCREEN_BUSY {
public:
    SCREEN_BUSY() { screen_set_busy(); }
    ~SCREEN_BUSY() { screen_unset_busy(); }
};

////////////////////////////////////////////////////////////////////////////////////////
// sqlite management
////////////////////////////////////////////////////////////////////////////////////////
#include <SQLiteCpp/SQLiteCpp.h>

namespace TWE {
    // datatype for DB with null.
    using DB_INTEGER = std::optional<int32_t>;   // int, 4bytes storage class
    using DB_TIMESTAMP = std::optional<int64_t>; // 64bit int type for timestamps
    using DB_REAL = std::optional<double>;       // double type for data.
    using DB_TEXT = std::optional<const char*>;  // text const char* (should be string class?)
    const std::nullopt_t DB_NULL = std::nullopt; // null (alias to std::nullopt)

    /**
     * small utility to output double values with fixed width.
     */
    struct _print_double {
        DB_REAL _d;
        _print_double(DB_REAL d) : _d(d) {}
    };

    /**
     * small utility to output double values with fixed width..
     * 
     * \param scr       output stream
     * \param rhs       _print_double object
     * \return          output stream
     */
    template <typename SCR>
    static SCR& operator << (SCR& scr, _print_double&& rhs) {
        auto& d = rhs._d;
        if (d) {
            double d_abs = std::abs(*d);
            if (d_abs > 0.0) d_abs += 0.0000005;
            
            // output a sign
            scr << ((*d < 0) ? '-' : ' ');

            // split into int part and frac part.
            double d_int;
            double d_frac = std::modf(d_abs, &d_int);

            // prepare integer presentation of int and frac part.
            int n_frac = int(d_frac * 1000000.);
            int n_int = d_abs < INT_MAX ? (int)d_int : INT_MAX; // if too large, set INT_MAX value.

            // output with 7 chars
            // note: unfortunatelly, referred printf library does not support output digits width for floating values.
            if (d_abs == 0.0) scr << "  0.000";
            else if (d_abs < 0.0001) scr << format("%1.1e", d_frac);
            else if (d_abs < 0.01) scr << format("%05f", d_frac);
            else if (n_int < 1000) scr << format("%3d.%03d", n_int, n_frac / 1000);
            else if (n_int < 10000) scr << format("%4d.%02d", n_int, n_frac / 10000);
            else if (n_int < 100000) scr << format("%5d.%01d", n_int, n_frac / 100000);
            else if (n_int < 10000000) scr << format("%7d", n_int);
            else scr << format("%1.1e", d_abs);
        }
        else {
            scr << " ---.---";
        }
        return scr;
    }


    /**
     * convert double to int32_t with range.
     * 
     * \param d
     * \param min_value
     * \param max_value
     * \param error_value
     * \return 
     */
    int32_t convert_double_to_int32_t(double d, int32_t min_value, int32_t max_value, int32_t error_value = 0x80000000) {
        if (d >= min_value && d <= max_value) {
            return int32_t(d);
        }
        else {
            return error_value;
        }
    }
}

/**
 * Wireless Sensor DB.
 * 
 * \param OS       output stream object for sqlite messages.
 */
struct WSnsDb {
    using QueryCmd = SmplBuf_ByteSL<1024>;


    
    /**
     * wrapper class of SQLite::Transaction.
     * - if commit, delete an instance explicitly.
     */
    struct Transaction {
        std::unique_ptr<SQLite::Transaction> _transaction;

        void commit() {
            if (_transaction) {
                _transaction->commit();
                _transaction.reset(nullptr);
            }
        }

        explicit operator bool() { return bool(_transaction); }

        Transaction() : _transaction() {}
    };

    /**
     * sensor data entity.
     */
    struct SENSOR_DATA {
        DB_INTEGER sid;
        DB_TIMESTAMP ts;        // UNIX epoch (The number of seconds since 1970-01-01 00:00:00 UTC)
        DB_INTEGER ts_msec;
        DB_INTEGER year;        // year part (localtime)
        DB_INTEGER month;       // month (localtime)
        DB_INTEGER day;         // day (localtime)
        DB_INTEGER hour;        // hour (localtime)
        DB_INTEGER lid;
        DB_INTEGER lqi;
        DB_INTEGER pkt_seq;
        DB_INTEGER pkt_type;
        DB_REAL value;
        DB_REAL value1;
        DB_REAL value2;
        DB_REAL value3;
        DB_INTEGER val_vcc_mv;
        DB_INTEGER val_dio;
        DB_INTEGER val_adc1_mv;
        DB_INTEGER val_adc2_mv;
        DB_INTEGER val_aux;
        DB_INTEGER ev_src;
        DB_INTEGER ev_id;
        DB_INTEGER ev_param;

        SENSOR_DATA()
            : sid()
            , ts()
            , ts_msec()
            , year(1970), month(1), day(1), hour(0)
            , lid()
            , lqi()
            , pkt_seq()
            , pkt_type()
            , value()
            , value1()
            , value2()
            , value3()
            , val_vcc_mv()
            , val_dio()
            , val_adc1_mv()
            , val_adc2_mv()
            , val_aux()
            , ev_src()
            , ev_id()
            , ev_param()
        {}

        /**
         * set timestampe(ts) as now.
         */
        void set_timestamp() {
            TWESYS::TweLocalTime t;
            t.now();
            ts = t.epoch;
        }
    };

public:

    /**
     * exception handler to output message to the stream.
     * 
     * \param e
     */
    void on_exception(std::exception& e) {
        _os << "SQLite exception: " << e.what() << crlf;
    }


    /**
     * Query with bindings: end of parameter pack.
     * 
     * \param query
     * \param idx
     */
    void _sql_statement(SQLite::Statement& query, int idx) {
        return;
    }

    /**
     * Query with bindings: extracting parameter pack.
     * 
     * \param query
     * \param idx           parameter(?)'s index starting from 0.
     * \param e             data entry.
     * \param ...tail
     */
    template <class... Tail, typename T>
    void _sql_statement(SQLite::Statement& query, int idx, std::optional<T> e, Tail&&... tail) {
        if (e) query.bind(idx, T(*e));
        else   query.bind(idx); // NULL value

        _sql_statement(query, ++idx, std::forward<Tail>(tail)...);
    }

    /**
     * Query with binding with variable arguments.
     * 
     * \param msg           SQL query string (e.g. "SELECT * FROM my_table WHERE (year=?) AND (month=?)")
     * \param ...tail       binding parameters (e.g. , 2022, 11)
     * \return              SQLite::Statement object
     */
    template <class... Tail>
    SQLite::Statement sql_statement(const char* msg, Tail&&... tail) {
        SQLite::Statement query(*_db, msg);

        _sql_statement(query, 1, std::forward<Tail>(tail)...);

        return std::move(query);
    }

    /**
     * Open database file.
     *
     * \return EXIT_SUCCESS
     * \return EXIT_FAILURE
     */
    int open(const char* db_filename) {
        if (db_filename) _db_filename << db_filename;

        try {
            _db.reset(new  SQLite::Database(_db_filename.c_str(), SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE));
        }
        catch (std::exception& e)
        {
            on_exception(e);
            return EXIT_FAILURE; // unexpected error : exit the example program
        }

        return EXIT_SUCCESS;
    }

    /**
     * Create necessary tables if not exist.
     *
     * \return EXIT_SUCCESS for success, EXIT_FAILURE for failure.
     */
    int prepare_tables() {
        try {
            // so far, it's not used but this table is intended to store meta inforamtion of each node.
            _db->exec("CREATE TABLE IF NOT EXISTS sensor_node ("
                "  sid INTEGER PRIMARY KEY" // module serial ID
                ", sid_text TEXT"           // TEXT format of SID
                ", desc TEXT"               // sensor node comment.
                ")"
            );

            // update every node data.
            _db->exec("CREATE TABLE IF NOT EXISTS sensor_last ("
                "  sid INTEGER PRIMARY KEY" // module serial ID
                ", ts INTEGER not null"     // timestamp of last data.
                ")"
            );

            // the sensor data.
            _db->exec("CREATE TABLE IF NOT EXISTS sensor_data ("
                "  _uqid integer primary key autoincrement" // unique id for PRIMARY KEY
                ", sid INTEGER not null"      // module serial ID
                ", ts INTEGER not null"       // timestamp (unix epoch)
                ", ts_msec INTEGER not null"  // timestamp (millisec part)
                ", year INTEGER not null"     // YEAR of localtime (note: not GMT)
                ", month INTEGER not null"    // MONTH of localtime
                ", day INTEGER not null"      // DAY of local time
                ", hour INTEGER not null"     // HOUR of local time
                ", lid INTEGER"               // Logical ID
                ", lqi INTEGER"               // LQI value (0..255)
                ", pkt_seq INTEGER"           // packet sequence number
                ", pkt_type INTEGER not null" // Corresponds to the sensor type included in the packet data.
                ", value REAL not null"       // primary sensor data
                ", value1 REAL"               // 2nd
                ", value2 REAL"               // 3rd
                ", value3 REAL"               // 4th
                ", val_vcc_mv INTEGER"        // Vcc
                ", val_dio INTEGER"           // Digital IO value (e.g. DIO or Magnet)
                ", val_adc1_mv INTEGER"       // ADC1
                ", val_adc2_mv INTEGER"       // ADC2  
                ", val_aux INTEGER"           // aux value
                ", ev_src INTEGER"            // event source
                ", ev_id INTEGER"             // event code (id)
                ", ev_param INTEGER"          // event parameter
                ")"
            );

            // place an index to make a query much faster.
            _db->exec("CREATE INDEX IF NOT EXISTS idx_ts ON sensor_data (sid, ts)");
#ifdef WSNS_DB_USE_YMDH
            _db->exec("CREATE INDEX IF NOT EXISTS idx_ymdh ON sensor_data (sid, year, month, day, hour)");
#endif
            
            
            // other settings
            _db->exec(
                "PRAGMA main.page_size = 4096;"
                "PRAGMA main.cache_size = 10000;"
                //"PRAGMA main.locking_mode = EXCLUSIVE;"
                "PRAGMA main.synchronous = NORMAL;"
                "PRAGMA main.journal_mode = WAL;"
                "PRAGMA main.cache_size = 5000;"
                "PRAGMA main.temp_store = MEMORY;"
            );
            
        }
        catch (std::exception& e)
        {
            on_exception(e);
            return EXIT_FAILURE; // unexpected error : exit the example program
        }

        return EXIT_SUCCESS;
    }

    /**
     * (FOR DEBUG) delete all tables.
     */
    void drop_tables() {
        try {
            _db->exec("DROP TABLE sensor_node");
        }
        catch (std::exception& e) { (void)e; }

        try {
            _db->exec("DROP TABLE sensor_last");
        }
        catch (std::exception& e) { (void)e; }

        try {
            _db->exec("DROP INDEX sensor_index");
        }
        catch (std::exception& e) { (void)e; }

        try {
            _db->exec("DROP TABLE sensor_data");
        }
        catch (std::exception& e) { (void)e; }
    }

    /**
     * insert sensor data.
     *
     * \param id        SID
     * \param desc      description of sensor node.
     * \return EXIT_SUCCESS for success, EXIT_FAILURE for failure.
     */
    int sensor_node_add(uint32_t sid, DB_TEXT desc) {
        auto scrbuzy = SCREEN_BUSY();

        try {
            // if desc is null, set dummy string
            if (!desc) desc = DB_TEXT("-no desc--");

            SmplBuf_ByteSL<63> sid_text;
            sid_text << format("%08X", sid);
            auto stmnt = sql_statement(
                "INSERT INTO sensor_node VALUES (?,?,?)"
                , DB_INTEGER(sid)
                , DB_TEXT(sid_text.c_str())
                , desc
            );

            stmnt.exec();
        }
        catch (std::exception& e) {
            on_exception(e);
            return EXIT_FAILURE; // unexpected error : exit the example program
        }

        return EXIT_SUCCESS;
    }

    /**
     * check if SIDs are present in sensor_node table.
     * if not, add an entry and set dummy desc.
     * 
     * note: before call, _node_seen has been set (by query_sorted_sensor_list_newer_first()).
     * 
     * \return 
     */
    int sensor_node_check() {
        try {
            auto stmnt = sql_statement(
                "SELECT * FROM sensor_node ORDER BY sid ASC"
            );

            SimpleBuffer<uint32_t> v_add(1024);

            while (stmnt.executeStep()) {
                if (!stmnt.getColumn(0).isNull()) {
                    uint32_t sid = uint32_t(stmnt.getColumn(0).getInt());
                    if (_node_seen(sid)) {
                        v_add.push_back(sid);
                    }
                }
            }

            for (auto x : _node_seen.v_nodes) {
                if (!std::binary_search(v_add.begin().raw_ptr(), v_add.end().raw_ptr(), x)) {
                    sensor_node_add(x, DB_TEXT());
                }
            }
        }
        catch (std::exception& e) {
            on_exception(e);
            return EXIT_FAILURE; // unexpected error : exit the example program
        }

        return EXIT_SUCCESS;
    }

    /**
     * insert sensor data.
     *
     * \param id
     * \param desc
     * \return EXIT_SUCCESS for success, EXIT_FAILURE for failure.
     */
    int sensor_last_add_or_update(uint32_t sid, DB_TIMESTAMP ts) {
        auto scrbuzy = SCREEN_BUSY();

        // replace latest ts of SID
        try {
            auto stmnt = sql_statement(
                "REPLACE INTO sensor_last VALUES (?,?)"
                , DB_INTEGER(int32_t(sid))
                , DB_TIMESTAMP(TWESYS::TweLocalTime::epoch_now())
            );

            stmnt.exec();

        }
        catch (std::exception& e) {
            on_exception(e);
            return EXIT_FAILURE; // unexpected error : exit the example program
        }

        // if SID is not in the sensor_node table.
        if (!_node_seen(sid)) {
            try {
#if 0
                auto stmnt_find_sid = sql_statement(
                    "SELECT * FROM sensor_node WHERE sid = ?"
                    , DB_INTEGER(int32_t(id))
                );

                if (stmnt_find_sid.executeStep()) {
                    if (stmnt_find_sid.getColumn(0).isNull()) b_newid = true;
                    else b_newid = false;
                }

#endif
                // add an new entry
                sensor_node_add(uint32_t(sid), DB_TEXT());
                
                // update internal vector
                _node_seen.append(sid);
            }
            catch (std::exception& e) {
                on_exception(e);
                return EXIT_FAILURE; // unexpected error : exit the example program
            }
        }

        return EXIT_SUCCESS;
    }

    /**
     * insert sensor data.
     *
     * \param d     sensor data struct.
     * \return      EXIT_SUCCESS for success, EXIT_FAILURE for failure.
     */
    int sensor_data_add(SENSOR_DATA& d) {
        auto scrbuzy = SCREEN_BUSY();
        QueryCmd cmd;

        TWESYS::TweLocalTime t;

        if (!d.ts) {
            t.now();

            d.ts = t.epoch;
            d.ts_msec = t.ms;
        }
        else {
            t.set_epoch(*d.ts);
        }

        d.year = DB_INTEGER::value_type(t.year);
        d.month = DB_INTEGER::value_type(t.month);
        d.day = DB_INTEGER::value_type(t.day);
        d.hour = DB_INTEGER::value_type(t.hour);

        try {
            auto stmnt = sql_statement(
                "INSERT INTO sensor_data VALUES(null"
                ",?" // sid
                ",?" // timestamp
                ",?" // ts_msec
                ",?,?,?,?" // year,month,day,hour
                ",?" // lid
                ",?" // lqi
                ",?" // pkt_seq
                ",?" // pkt_type
                ",?,?,?,?"   // value...value3
                ",?,?,?,?,?" // val_vcc_mv, val_mag, val_adc1_mv, val_adc2_mv, val_aux
                ",?,?,?"     // event src, id, param
                ")"
                , d.sid
                , d.ts
                , d.ts_msec
                , d.year, d.month, d.day, d.hour
                , d.lid
                , d.lqi
                , d.pkt_seq
                , d.pkt_type
                , d.value, d.value1, d.value2, d.value3
                , d.val_vcc_mv, d.val_dio, d.val_adc1_mv, d.val_adc2_mv, d.val_aux
                , d.ev_src, d.ev_id, d.ev_param
            );

            stmnt.exec();
        } 
        catch (std::exception& e)
        {
            on_exception(e);
            return EXIT_FAILURE;
        }

        // upate latest tick count
        if (sensor_last_add_or_update(*d.sid, *d.ts) != EXIT_SUCCESS) {
            return EXIT_FAILURE;
        }

        return EXIT_SUCCESS;
    }

    /**
     * execute query sensor data statement.
     * 
     * \param query
     * \param hndl_query
     */
    void _query_core(SQLite::Statement& query, std::function<void(SENSOR_DATA&)>& hndl_query) {
    //template <typename TF> void _query_core(SQLite::Statement & query, TF&& hndl_query) {
        #define __TWE_GET_COL_IDX(n) int i_##n = query.getColumnIndex(#n)
        __TWE_GET_COL_IDX(sid);
        __TWE_GET_COL_IDX(ts);
        __TWE_GET_COL_IDX(ts_msec);
        __TWE_GET_COL_IDX(year);
        __TWE_GET_COL_IDX(month);
        __TWE_GET_COL_IDX(day);
        __TWE_GET_COL_IDX(hour);
        __TWE_GET_COL_IDX(lid);
        __TWE_GET_COL_IDX(lqi);
        __TWE_GET_COL_IDX(pkt_seq);
        __TWE_GET_COL_IDX(pkt_type);
        __TWE_GET_COL_IDX(value);
        __TWE_GET_COL_IDX(value1);
        __TWE_GET_COL_IDX(value2);
        __TWE_GET_COL_IDX(value3);
        __TWE_GET_COL_IDX(val_vcc_mv);
        __TWE_GET_COL_IDX(val_dio);
        __TWE_GET_COL_IDX(val_adc1_mv);
        __TWE_GET_COL_IDX(val_adc2_mv);
        __TWE_GET_COL_IDX(val_aux);
        __TWE_GET_COL_IDX(ev_src);
        __TWE_GET_COL_IDX(ev_id);
        __TWE_GET_COL_IDX(ev_param);
        #undef __GET_COLMN_IDX

        while (query.executeStep()) {
            #define __TWE_DB_IS_NULL(N) if (!query.getColumn(i_##N).isNull())
            SENSOR_DATA d;
            d.sid = DB_INTEGER::value_type(query.getColumn(i_sid));
            d.ts = DB_TIMESTAMP::value_type(query.getColumn(i_ts).getInt64());
            d.ts_msec = DB_INTEGER::value_type(query.getColumn(i_ts_msec));
            d.year = DB_INTEGER::value_type(query.getColumn(i_year));
            d.month = DB_INTEGER::value_type(query.getColumn(i_month));
            d.day = DB_INTEGER::value_type(query.getColumn(i_day));
            d.hour = DB_INTEGER::value_type(query.getColumn(i_hour));
            __TWE_DB_IS_NULL(lid) d.lid = DB_INTEGER::value_type(query.getColumn(i_lid));
            __TWE_DB_IS_NULL(lqi) d.lqi = DB_INTEGER::value_type(query.getColumn(i_lqi));
            __TWE_DB_IS_NULL(pkt_seq) d.pkt_seq = DB_INTEGER::value_type(query.getColumn(i_pkt_seq));
            __TWE_DB_IS_NULL(pkt_type) d.pkt_type = DB_INTEGER::value_type(query.getColumn(i_pkt_type));
            d.value = DB_REAL::value_type(query.getColumn(i_value).getDouble());
            __TWE_DB_IS_NULL(value1) d.value1 = DB_REAL::value_type(query.getColumn(i_value1).getDouble());
            __TWE_DB_IS_NULL(value2) d.value2 = DB_REAL::value_type(query.getColumn(i_value2).getDouble());
            __TWE_DB_IS_NULL(value3) d.value3 = DB_REAL::value_type(query.getColumn(i_value3).getDouble());
            __TWE_DB_IS_NULL(val_vcc_mv) d.val_vcc_mv = DB_INTEGER::value_type(query.getColumn(i_val_vcc_mv));
            __TWE_DB_IS_NULL(val_dio) d.val_dio = DB_INTEGER::value_type(query.getColumn(i_val_dio));
            __TWE_DB_IS_NULL(val_adc1_mv) d.val_adc1_mv = DB_INTEGER::value_type(query.getColumn(i_val_adc1_mv));
            __TWE_DB_IS_NULL(val_adc2_mv) d.val_adc2_mv = DB_INTEGER::value_type(query.getColumn(i_val_adc2_mv));
            __TWE_DB_IS_NULL(val_aux) d.val_aux = DB_INTEGER::value_type(query.getColumn(i_val_aux));
            __TWE_DB_IS_NULL(ev_src) d.ev_src = DB_INTEGER::value_type(query.getColumn(i_ev_src));
            __TWE_DB_IS_NULL(ev_id) d.ev_id = DB_INTEGER::value_type(query.getColumn(i_ev_id));
            __TWE_DB_IS_NULL(ev_param) d.ev_param = DB_INTEGER::value_type(query.getColumn(i_ev_param));
            #undef __TWE_DB_IS_NULL

            hndl_query(d);
        }
    }
    /**
     * query sensor data by SDI and range of timestamp.
     * - use like below:
     *   ts_now = xxx; // the current timestamp.
     *   v_sensor_data = xxx; // array data of sensor data.
     *   db.query_sensor_data(0x8012345, ts_now, ts_now - 3600, 
     *      [&](WSnsDb::SENSOR_DATA& d) { v_sensor_data.push_back(d); } // lambda expression.
     *   );
     *
     * \param sid       module SID
     * \param ts_start  timestamp of range start
     * \param ts_end    timestamp of rande end
     * \param           external handler function(or lambda expression) which is called each entry.
     * \return          EXIT_SUCCESS for success, EXIT_FAILURE for failure.
     */
    int query_sensor_data(uint32_t sid, uint64_t ts_start, uint64_t ts_end, std::function<void(SENSOR_DATA&)> hndl_query, int n_mode = 0) {
    //template <typename TF> int query_sensor_data(uint32_t sid, uint64_t ts_start, uint64_t ts_end, TF&& hndl_query) {
        auto scrbuzy = SCREEN_BUSY();

        try {
            const char stmt_query[3][256] = {
                "SELECT * FROM sensor_data WHERE (sid=?) and (ts BETWEEN ? and ?)",
                "SELECT * FROM sensor_data WHERE (sid=?) and (ts BETWEEN ? and ?) ORDER BY ts ASC", // sorted 
                "SELECT * FROM sensor_data WHERE (sid=?) and (ts BETWEEN ? and ?) ORDER BY random() LIMIT 512" // random order and limit count
            };

            auto query = sql_statement(
                stmt_query[n_mode]
                , DB_INTEGER(int32_t(sid))
                , DB_TIMESTAMP(ts_start)
                , DB_TIMESTAMP(ts_end)
            );

            _query_core(query, hndl_query);
        }
        catch (std::exception& e)
        {
            on_exception(e);
            return EXIT_FAILURE;
        }

        return EXIT_SUCCESS;
    }

    /**
     * query data by year/month/day/hour.
     * 
     * \param sid
     * \param year
     * \param month
     * \param day
     * \param hour
     * \param hndl_query
     * \return 
     */
    int query_sensor_data_by_hour(uint32_t sid, int16_t year, int16_t month, int16_t day, int16_t hour, std::function<void(SENSOR_DATA&)> hndl_query) {
        auto scrbuzy = SCREEN_BUSY();

        try {
#ifdef WSNS_DB_USE_YMDH
            auto query = sql_statement(
                "SELECT * FROM sensor_data"
                " WHERE (sid=?) and (year=?) and (month=?) and (day=?) and (hour=?)"
                " ORDER BY ts ASC"
                , DB_INTEGER(int32_t(sid))
                , DB_INTEGER(year)
                , DB_INTEGER(month)
                , DB_INTEGER(day)
                , DB_INTEGER(hour)
            );
            
#else
            // this query is much faster than combination of year/month/day/hour.
            TWESYS::TweLocalTime t;

            t.year = year;
            t.month = month;
            t.day = day;
            t.hour = hour;
            t.minute = 0;
            t.second = 0;
            t.get_epoch();

            auto query = sql_statement(
                "SELECT * FROM sensor_data"
                " WHERE (sid=?) and (ts BETWEEN ? and ?)"
                " ORDER BY ts ASC"
                , DB_INTEGER(int32_t(sid))
                , DB_TIMESTAMP(int64_t(t.epoch))
                , DB_TIMESTAMP(int64_t(t.epoch + 3600 - 1))
            );
#endif
            _query_core(query, hndl_query);
        }
        catch (std::exception& e)
        {
            on_exception(e);
            return EXIT_FAILURE;
        }

        return EXIT_SUCCESS;
    }

    /**
     * query data by year/month/day.
     *
     * \param sid
     * \param year
     * \param month
     * \param day
     * \param hour
     * \param hndl_query
     * \return
     */
    int query_sensor_data_by_day(uint32_t sid, int16_t year, int16_t month, int16_t day, std::function<void(SENSOR_DATA&)> hndl_query) {
        auto scrbuzy = SCREEN_BUSY();

        try {
#ifdef WSNS_DB_USE_YMDH
            auto query = sql_statement(
                "SELECT * FROM sensor_data"
                " WHERE (sid=?) and (year=?) and (month=?) and (day=?)"
                " ORDER BY ts ASC"
                , DB_INTEGER(int32_t(sid))
                , DB_INTEGER(year)
                , DB_INTEGER(month)
                , DB_INTEGER(day)
            );
#else
            TWESYS::TweLocalTime t;

            t.year = year;
            t.month = month;
            t.day = day;
            t.hour = 0;
            t.minute = 0;
            t.second = 0;
            t.get_epoch();

            auto query = sql_statement(
                "SELECT * FROM sensor_data"
                " WHERE (sid=?) and (ts BETWEEN ? and ?)"
                " ORDER BY ts ASC"
                , DB_INTEGER(int32_t(sid))
                , DB_TIMESTAMP(int64_t(t.epoch))
                , DB_TIMESTAMP(int64_t(t.epoch+86400-1))
            );
#endif

            _query_core(query, hndl_query);
        }
        catch (std::exception& e)
        {
            on_exception(e);
            return EXIT_FAILURE;
        }

        return EXIT_SUCCESS;
    }

    /**
     * find oldest timestamp and newest timestamp of SID.
     * 
     * \param sid
     * \param oldest
     * \param newest
     * \return 
     */
    int query_oldest_and_newest(uint32_t sid, DB_TIMESTAMP &oldest, DB_TIMESTAMP &newest) {
        try {
            {
                auto query = sql_statement("SELECT MIN(ts) FROM sensor_data WHERE sid = ?", DB_INTEGER(int32_t(sid)));
                if (query.executeStep()) {
                    auto col = query.getColumn(0);
                    if (!col.isNull()) {
                        oldest = DB_TIMESTAMP(query.getColumn(0).getInt64());
                    }
                }
            }

            {
                auto query = sql_statement("SELECT MAX(ts) FROM sensor_data WHERE sid = ?", DB_INTEGER(int32_t(sid)));
                if (query.executeStep()) {
                    auto col = query.getColumn(0);
                    if (!col.isNull()) {
                        newest = DB_TIMESTAMP(query.getColumn(0).getInt64());
                    }
                }
            }

            // both shall have value.
            if (!oldest || !newest) {
                oldest = DB_NULL;
                newest = DB_NULL;
                return EXIT_FAILURE;
            }
        }
        catch (std::exception& e)
        {
            on_exception(e);
            return EXIT_FAILURE;
        }

        return EXIT_SUCCESS;
    }

    /**
     * List the years that contain the sensor data specified by the SID.
     * 
     * \param sid           module SDI
     * \param hdnl_query    external handler function(or lambda expression) which is called each entry.
     * \return              EXIT_SUCCESS for success, EXIT_FAILURE for failure.
     */
    int query_recorded_years_of_sensor_data(uint32_t sid, std::function<void(int16_t year)> hdnl_query) {
        auto scrbuzy = SCREEN_BUSY();

        try {
#ifdef WSNS_DB_USE_YMDH
            auto query = sql_statement("SELECT DISTINCT year FROM sensor_data WHERE sid = ? ORDER BY year ASC", DB_INTEGER(int32_t(sid)));
            auto i_year = query.getColumnIndex("year");

            while (query.executeStep()) {
                auto year = DB_INTEGER::value_type(query.getColumn(i_year));
                hdnl_query(int16_t(year));
            }
#else
            // TWESYS::TweLocalTime t_now;
            // t_now.now();
            //int year_b = t_now.year - 10;
            //int year_e = t_now.year + 1;

            int year_b = -1;
            int year_e = -1;

            TWESYS::TweLocalTime  t_max;
            TWESYS::TweLocalTime  t_min;

            // query max and min of year (DD NOT query MAX() and MIN() within a single SQL statement, which is much slower)
            //uint32_t t0 = millis();
            {
                auto query = sql_statement("SELECT MIN(ts) FROM sensor_data WHERE sid = ?", DB_INTEGER(int32_t(sid)));
                if (query.executeStep()) t_min.epoch = int64_t(query.getColumn(0));
            }
            {
                auto query = sql_statement("SELECT MAX(ts) FROM sensor_data WHERE sid = ?", DB_INTEGER(int32_t(sid)));
                if (query.executeStep()) t_max.epoch = int64_t(query.getColumn(0));
            }
            if (t_min.epoch == 0 || t_max.epoch == 0) { // on error, query.getColumn() is not Integer Type, resulting 0 is set.
                return EXIT_FAILURE;
            }
            //WrtCon << format("<SELECT max(ts) min(ts): %dms>", millis() - t0);

            t_min.set_epoch(t_min.epoch);
            t_max.set_epoch(t_max.epoch);

            year_b = t_min.year;
            year_e = t_max.year;

            TWESYS::TweLocalTime t, tn;
            
            t.month = 1;
            t.day = 1;
            t.hour = 0;
            t.minute = 0;
            t.second = 0;

            tn.month = 1;
            tn.day = 1;
            tn.hour = 0;
            tn.minute = 0;
            tn.second = 0;

            hdnl_query(year_b); //year_b is already confirmed
            if (year_e - year_b >= 2) {
                for (int i = year_b + 1; i < year_e; i++) { // check year_b + 1 to year_e - 1
                    t.year = i;
                    tn.year = i + 1;

                    t.get_epoch();
                    tn.get_epoch();

                    auto query = sql_statement("SELECT month from sensor_data WHERE (sid=?) AND (ts BETWEEN ? and ?)"
                        , DB_INTEGER(sid)
                        , DB_TIMESTAMP(t.epoch)
                        , DB_TIMESTAMP(tn.epoch - 1));

                    while (query.executeStep()) {
                        hdnl_query(i);
                        break;
                    }
                }
            }
            if (year_b != year_e) hdnl_query(year_e); //year_e is already confirmed
#endif
        }
        catch (std::exception& e)
        {
            on_exception(e);
            return EXIT_FAILURE;
        }

        return EXIT_SUCCESS;
    }

    /**
     * List the "months" that contain the sensor data from the wireless module 
     * specified by the SID in the specified year.
     *
     * \param sid           module SID
     * \param year          year to find sensor data.
     * \param hdnl_query    external handler function(or lambda expression) which is called each entry.
     * \return              EXIT_SUCCESS for success, EXIT_FAILURE for failure.
     */
    int query_recorded_months_of_sensor_data(uint32_t sid, int16_t year, std::function<void(int16_t month)> hdnl_query) {
        auto scrbuzy = SCREEN_BUSY();

        try {
#ifdef WSNS_DB_USE_YMDH
            auto query = sql_statement("SELECT DISTINCT month FROM sensor_data WHERE (sid=?) AND (year=?) ORDER BY month ASC", DB_INTEGER(int32_t(sid)), DB_INTEGER(year));
            auto i_month = query.getColumnIndex("month");

            while (query.executeStep()) {
                auto month = DB_INTEGER::value_type(query.getColumn(i_month));
                hdnl_query(int16_t(month));
            }
#else
            TWESYS::TweLocalTime t, tn;
            t.year = year;
            t.day = 1;
            t.hour = 0;
            t.minute = 0;
            t.second = 0;

            tn.year = year;
            tn.day = 1;
            tn.hour = 0;
            tn.minute = 0;
            tn.second = 0;

            for (int i = 1; i <= 12; i++) {
                t.month = i;
                t.get_epoch();

                tn.month = i + 1;
                if (tn.month == 13) {
                    tn.year = t.year + 1;
                    tn.month = 1;
                }
                tn.get_epoch();

                auto query = sql_statement("SELECT month from sensor_data WHERE (sid=?) AND (ts BETWEEN ? and ?)"
                    , DB_INTEGER(sid)
                    , DB_TIMESTAMP(t.epoch)
                    , DB_TIMESTAMP(tn.epoch - 1));

                while (query.executeStep()) {
                    hdnl_query(i);
                    break;
                }
            }
#endif
        }
        catch (std::exception& e)
        {
            on_exception(e);
            return EXIT_FAILURE;
        }

        return EXIT_SUCCESS;
    }

    /**
     * List the "days" that contain the sensor data from the wireless module
     * specified by the SID in the specified year and month.
     *
     * \param sid           module SID
     * \param year          year to find sensor data.
     * \param month         month to find sensor data.
     * \param hdnl_query    external handler function(or lambda expression) which is called each entry.
     * \return              EXIT_SUCCESS for success, EXIT_FAILURE for failure.
     */
    int query_recorded_days_of_sensor_data(uint32_t sid, int16_t year, int16_t month, std::function<void(int16_t day)> hdnl_query) {
        auto scrbuzy = SCREEN_BUSY();
        try {
#if WSNS_DB_USE_YMDH
            auto query = sql_statement("SELECT DISTINCT day FROM sensor_data WHERE (sid=?) AND (year=?) AND (month=?) ORDER BY day ASC", DB_INTEGER(int32_t(sid)), DB_INTEGER(year), DB_INTEGER(month));
            auto i_day = query.getColumnIndex("day");

            while (query.executeStep()) {
                auto day = DB_INTEGER::value_type(query.getColumn(i_day));
                hdnl_query(int16_t(day));
            }
#else
            TWESYS::TweLocalTime t, tn;
            t.year = year;
            t.month = month;
            t.day = 1;
            t.hour = 0;
            t.minute = 0;
            t.second = 0;
            t.get_epoch();

            tn.year = year;
            tn.month = month + 1;
            if (month == 13) {
                tn.month = 1;
                tn.year++;
            }
            tn.day = 1;
            tn.hour = 0;
            tn.minute = 0;
            tn.second = 0;
            tn.get_epoch();

            for (int i = 1; i <= 31 && t.epoch < tn.epoch; i++) {
                t.day = i;

                auto query = sql_statement("SELECT month from sensor_data WHERE (sid=?) AND (ts BETWEEN ? and ?)"
                    , DB_INTEGER(sid)
                    , DB_TIMESTAMP(t.epoch)
                    , DB_TIMESTAMP(t.epoch + 86400 - 1));
                 
                while (query.executeStep()) {
                    hdnl_query(i);
                    break;
                }

                t.epoch += 86400;
            }
#endif
        }
        catch (std::exception& e)
        {
            on_exception(e);
            return EXIT_FAILURE;
        }

        return EXIT_SUCCESS;
    }

    /**
     * List the "hours" that contain the sensor data from the wireless module
     * specified by the SID in the specified year, month and day.
     *
     * \param sid           module SID
     * \param year          year to find sensor data.
     * \param month         month to find sensor data.
     * \param month         day to find sensor data.
     * \param hdnl_query    external handler function(or lambda expression) which is called each entry.
     * \return              EXIT_SUCCESS for success, EXIT_FAILURE for failure.
     */
    int query_recorded_hours_of_sensor_data(uint32_t sid, int16_t year, int16_t month, int16_t day, std::function<void(int16_t hour)> hdnl_query) {
        auto scrbuzy = SCREEN_BUSY();
        try {
#if WSNS_DB_USE_YMDH
            auto query = sql_statement("SELECT DISTINCT hour FROM sensor_data WHERE (sid=?) AND (year=?) AND (month=?) AND (day=?) ORDER BY hour ASC"
                                     , DB_INTEGER(int32_t(sid)), DB_INTEGER(year), DB_INTEGER(month), DB_INTEGER(day));

            auto i_hour = query.getColumnIndex("hour");

            while (query.executeStep()) {
                auto hour = DB_INTEGER::value_type(query.getColumn(i_hour));
                hdnl_query(int16_t(hour));
            }
#else
            TWESYS::TweLocalTime t;
            t.year = year;
            t.month = month;
            t.day = day;
            t.hour = 0;
            t.minute = 0;
            t.second = 0;
            t.get_epoch();

            for (int i = 0; i <= 23; i++) {
                auto query = sql_statement("SELECT month from sensor_data WHERE (sid=?) AND (ts BETWEEN ? and ?)"
                    , DB_INTEGER(sid)
                    , DB_TIMESTAMP(t.epoch + i * 3600)
                    , DB_TIMESTAMP(t.epoch + (i + 1) * 3600 - 1));

                while (query.executeStep()) {
                    hdnl_query(i);
                    break;
                }
            }
#endif
        }
        catch (std::exception& e)
        {
            on_exception(e);
            return EXIT_FAILURE;
        }

        return EXIT_SUCCESS;
    }

    /**
     * List the sensor SIDs ordered by timestamp.
     * 
     * \param hndl_query    external handler function(or lambda expression) which is called each entry.
     * \return 
     */
    int query_sorted_sensor_list_newer_first(std::function<void(uint32_t sid, uint64_t ts)> hndl_query) {
        auto scrbuzy = SCREEN_BUSY();

        try {
            auto query = sql_statement("SELECT * FROM sensor_last ORDER BY ts DESC");

            while (query.executeStep()) {
                hndl_query(
                    uint32_t(query.getColumn(0).getInt()),
                    uint64_t(query.getColumn(1).getInt64())
                );

                // update internal list
                _node_seen.append(uint32_t(query.getColumn(0).getInt()));
            }
        }
        catch (std::exception& e)
        {
            on_exception(e);
            return EXIT_FAILURE;
        }

        return EXIT_SUCCESS;
    }

    /**
     * Get the latest timestamp of sensor data with SID..
     * 
     * \param sid
     * \param ts_result
     * \return 
     */
    int query_latest_ts(uint32_t sid, DB_TIMESTAMP& ts_result) {
        ts_result = DB_NULL;

        try {
            auto query = sql_statement("SELECT * FROM sensor_last WHERE sid = ?"
                , DB_INTEGER(int32_t(sid))
            );

            if (query.executeStep()) {
                ts_result = query.getColumn("ts");
            }
        }
        catch (std::exception& e)
        {
            on_exception(e);
            return EXIT_FAILURE;
        }

        return EXIT_SUCCESS;
    }

    /**
     * Find previous entry from specified timestamp.
     * 
     * \param sid
     * \param ts_ref
     * \param ts_result
     * \return 
     */
    int query_previous_ts(uint32_t sid, uint64_t ts_ref, DB_TIMESTAMP& ts_result) {
        auto scrbuzy = SCREEN_BUSY();

        ts_result = DB_NULL;

        try {
            auto query = sql_statement("SELECT MAX(ts) FROM sensor_data WHERE (sid = ?) AND (ts BETWEEN 0 AND ?)"
                , DB_INTEGER(int32_t(sid))
                , DB_TIMESTAMP(int64_t(ts_ref - 1))
            );

            if (query.executeStep()) {
                const auto& col = query.getColumn(0);
                if (col.isInteger()) {
                    ts_result = DB_TIMESTAMP(int64_t(col.getInt64()));
                }
            }
        }
        catch (std::exception& e)
        {
            on_exception(e);
            return EXIT_FAILURE;
        }

        return EXIT_SUCCESS;
    }

    /**
     * Find next entry from specified timestamp.
     * 
     * \param sid
     * \param ts_ref
     * \param ts_result
     * \return 
     */
    int query_next_ts(uint32_t sid, uint64_t ts_ref, DB_TIMESTAMP& ts_result) {
        auto scrbuzy = SCREEN_BUSY();

        ts_result = DB_NULL;

        try {
            auto query = sql_statement("SELECT MIN(ts) FROM sensor_data WHERE (sid = ?) AND (ts BETWEEN ? AND ?)"
                , DB_INTEGER(int32_t(sid))
                , DB_TIMESTAMP(int64_t(ts_ref + 1))
                , DB_TIMESTAMP(253402300799ll) // ts big enough (9999/12/31)
            );

            if (query.executeStep()) {
                const auto& col = query.getColumn(0);
                if (col.isInteger()) {
                    ts_result = DB_TIMESTAMP(int64_t(col.getInt64()));
                }
            }

        }
        catch (std::exception& e)
        {
            on_exception(e);
            return EXIT_FAILURE;
        }

        return EXIT_SUCCESS;
    }
    
    /**
     * query description of SID stored in sensor_node table.
     * 
     * \param sid
     * \param strbuff_to_append
     * \return 
     */
    int query_sid_string(uint32_t sid, SmplBuf_WChar& strbuff_to_append) {
        try {
            auto query = sql_statement("SELECT * FROM sensor_node WHERE (sid = ?)", DB_INTEGER(int32_t(sid)));

            if (query.executeStep()) {
                const auto& col = query.getColumn(2);
                if (col.isText()) {
                    auto p = col.getText();
                    strbuff_to_append << p;
                }
            }
        }
        catch (std::exception& e)
        {
            on_exception(e);
            return EXIT_FAILURE;
        }

        return EXIT_SUCCESS;
    }

    /**
     * create transaction object.
     * - use as below.
     *   void foo() {
     *     if (auto&& x = db.get_transaction_obj()) {
     *       ...          // some db process
     *       x->commit(); // finally, commit the process.
     *     }
     *   }
     * 
     * \return unique_ptr of transaction obj.
     */
    Transaction get_transaction_obj() {
        Transaction trs;
        trs._transaction.reset(new SQLite::Transaction(*_db));
        return std::move(trs); // can be RVO.
    }

    /**
     * table of SIDS which is seen.
     */
    struct _NODE_SEEN {
        SimpleBuffer<uint32_t> v_nodes; // node id list, which is 

        /**
         * add an entry.
         *
         */
        void append(uint32_t sid_add) {
            if (!operator()(sid_add)) {
                v_nodes.push_back(sid_add);
            }

            SmplBuf_Sort(v_nodes, [](uint32_t a, uint32_t b) { return a > b; });
        }

        /**
         * .
         *
         * \param sid_find
         */
        bool operator() (uint32_t sid_find) {
            bool res = std::binary_search(v_nodes.begin().raw_ptr(), v_nodes.end().raw_ptr(), sid_find);
            return res;
        }

        _NODE_SEEN() : v_nodes(1024) {}
    };

    WSnsDb(TWE::IStreamOut& os)
        : _db_filename()
        , _db()
        , _os(os)
        , _node_seen()
    {
    }

    ~WSnsDb()
    {
        ;
    }

private:
    SmplBuf_ByteSL<256> _db_filename;       // DB filename
    std::unique_ptr<SQLite::Database> _db;  // the DB
    TWE::IStreamOut& _os;                   // output stream for message

    _NODE_SEEN _node_seen;
};

////////////////////////////////////////////////////////////////////////////////////////
// SCR_WSNS_DB
////////////////////////////////////////////////////////////////////////////////////////
struct App_CUE::SCR_WSNS_DB : public APP_HANDLR_DC, public TWE::APP_HNDLR<App_CUE::SCR_WSNS_DB> {
public:
    
	static const int CLS_ID = App_CUE::PAGE_ID::PAGE_WSNS_DB;

	App_CUE& _app;
	TWE_WidSet_Buttons _btns;
	int _pkt_rcv_ct;

	// object references to the App_ARIA
    TWETerm_M5_Console& the_screen;
	ITerm& the_screen_b;
	IParser& parse_ascii;

	// database
    std::unique_ptr<WSnsDb> _db;
    WSnsDb::Transaction _db_transaction;

    // loop seconds
    uint32_t _sec;
    TWESYS::TweLocalTime _lt_now;

    /**
     * Managing horizontal scrollbar.
     */
    class TWE_GUI_ScrollBarH {
    private:
        Rect _rg_all;				// whole area in the screen
        Rect _rg_bar;				// bar area (excluding frame)
        Rect _rg_btn;				// bar button area
        int32_t _var_min;			// minimum numeric value
        int32_t _var_max;			// maximum numeric value
        int32_t _var_view_start;	// numeric value of screen left.
        int32_t _var_view_last;		// numeric value of screen right.

    public:
        TWE_GUI_ScrollBarH()
            : _rg_all(), _rg_bar(), _rg_btn()
            , _var_min(0), _var_max(100)
            , _var_view_start(40), _var_view_last(60)
        {
        }

        /**
         * calculate bar area from give area.
         *
         * \param rg	whole area of scrollbar.
         */
        void set_draw_area(Rect rg) {
            _rg_all = rg;

            // bar area : 1px inside of thee whole area.
            _rg_bar.h = _rg_all.h - 2;
            _rg_bar.w = _rg_all.w - 2;
            _rg_bar.x = _rg_all.x + 1;
            _rg_bar.y = _rg_all.y + 1;
            _rg_btn = _rg_bar;
        }

        /**
         * calculate bar button position from numeric values.
         *
         * \param start		numeric value of screen left.
         * \param last		numeric value of screen right.
         * \param v_min		minimum numeric value.
         * \param v_max		maximum numeric value.
         */
        void set_value(int32_t start, int32_t last, int32_t v_min, int32_t v_max) {
            if (!(v_min < v_max)) return;

            _var_view_start = start;
            _var_view_last = last;
            _var_min = v_min;
            _var_max = v_max;

            int32_t w = (last - start) * _rg_bar.w / (v_max - v_min);
            int32_t s = (start - v_min) * _rg_bar.w / (v_max - v_min);
            int32_t l = (last - v_min) * _rg_bar.w / (v_max - v_min);

            _rg_btn.w = l - s;
            _rg_btn.h = _rg_bar.h;
            _rg_btn.x = _rg_bar.x + s;
            _rg_btn.y = _rg_bar.y;
        }

        /**
         * rendering the scrollbar.
         * - set_value() should be perfomed before call.
         */
        void update_view() {
            if (_rg_btn.w < _rg_bar.w) {
                M5.Lcd.drawRect(_rg_all.x, _rg_all.y, _rg_all.w, _rg_all.h, color565(120, 120, 120)); // frame
                M5.Lcd.fillRect(_rg_bar.x, _rg_bar.y, _rg_bar.w, _rg_bar.h, color565(80, 80, 80));	  // bar bg
                M5.Lcd.fillRect(_rg_btn.x, _rg_btn.y, _rg_btn.w, _rg_btn.h, color565(160, 160, 160)); // bar button
            }
            else {
                // inactivated (value range and view range is the same)
                M5.Lcd.drawRect(_rg_all.x, _rg_all.y, _rg_all.w, _rg_all.h, color565(120, 120, 120)); // frame
                M5.Lcd.fillRect(_rg_bar.x, _rg_bar.y, _rg_bar.w, _rg_bar.h, color565(80, 80, 80));	  // bar bg
            }
        }
    };

    /**
     * manages change between sub-screens.
     */
    struct _SCRN_MGR {
        // subscreen IDs
        static const int SUBS_VOID = -1;
        static const int SUBS_LIST_NODES = 1;
        static const int SUBS_LIST_YEARS = 2;
        static const int SUBS_LIST_MONTHS = 3;
        static const int SUBS_LIST_DAYS = 4;
        static const int SUBS_LIST_HOURS = 5;
        static const int SUBS_LIVE_VIEW = 6;
        static const int SUBS_D_DAY = 7;
        static const int SUBS_DEBUG = 16;
        static const int SUBS_FATAL = 254;
        static const int SUBS_PREVIOUS = 255;

        int32_t _subscreen_change_req;
        int32_t _subscreen_now;
        int32_t _subscreen_previous;

        /**
         * request to change sub-screen.
         * 
         * \param new_screen_id
         */
        void screen_change_request(int new_screen_id) {
            _subscreen_change_req = new_screen_id;
        }

        /**
         * perform change of sub-screen.
         * note: this must be placed at the end of SCR_WSNS_DB::loop().
         * 
         * \param base
         */
        void screen_change_perform(SCR_WSNS_DB& base) {
            if (_subscreen_change_req != SUBS_VOID) {
                bool b_changed = true;

                if (_subscreen_change_req == SUBS_PREVIOUS && _subscreen_previous != SUBS_VOID) {
                    _subscreen_change_req = _subscreen_previous;
                }

                switch (_subscreen_change_req) {
                    #define __MWM5_MCR_SUBSCREEN_CASE(N) case N::CLS_ID: base.new_hndlr(&SCR_WSNS_DB::hndr_subs<N>); break
                    __MWM5_MCR_SUBSCREEN_CASE(subscr_list_nodes);
                    __MWM5_MCR_SUBSCREEN_CASE(subscr_list_years);
                    __MWM5_MCR_SUBSCREEN_CASE(subscr_list_months);
                    __MWM5_MCR_SUBSCREEN_CASE(subscr_list_days);
                    //__MWM5_MCR_SUBSCREEN_CASE(subscr_list_hours);
                    __MWM5_MCR_SUBSCREEN_CASE(subscr_live_view);;
                    __MWM5_MCR_SUBSCREEN_CASE(subscr_24hrs);
                    __MWM5_MCR_SUBSCREEN_CASE(subscr_fatal);
#ifdef WSNS_DEBUG_FUNCTIONS
                    __MWM5_MCR_SUBSCREEN_CASE(subscr_debug);
#endif
                    default: b_changed = false;  break;
                }

                if (b_changed) {
                    _subscreen_previous = _subscreen_now;
                    _subscreen_now = _subscreen_change_req;
                }
                _subscreen_change_req = SUBS_VOID;
            }
        }

        _SCRN_MGR() : _subscreen_change_req(SUBS_VOID), _subscreen_now(SUBS_VOID), _subscreen_previous(SUBS_VOID) {}
    } _scr_sub;
    
    /**
     * stores selected node details.
     */
    struct _NODE_INFO {
        SCR_WSNS_DB& base;      // easy to access parent class object

        uint32_t sid;           // module SID

        int16_t year;           // selected year
        int16_t month;          // selected month
        int16_t day;            // selected day
        int16_t hour;           // selected hour (no use now)

        DB_TIMESTAMP ts_oldest, ts_newest; // timestamp of oldest/newest data.

        bool b_live_update; // true if showing live status.

        TWEUTILS::FixedQueue<WSnsDb::SENSOR_DATA> live_history; // live data copy (for sensor datas which has not been committed yet)

        /**
         * prepare an instance of _node with specified SID.
         *
         * \param sid
         */
        void set_sid(uint32_t sid_set) {
            // if new node, renew _node instance.
            if (sid != sid_set) {
                live_history.clear();
            }
            sid = sid_set; // set sid

            if (sid != 0) {
                // query newest timestamp and oldest timestamp
                base._db->query_oldest_and_newest(sid, ts_oldest, ts_newest);
            }
            else {
                ts_oldest = DB_NULL;
                ts_newest = DB_NULL;
            }
        }

        /**
         * query latest data with SID.
         * - set sid in advance
         */
        bool set_latest_date() {
            auto& db = *base._db;
            if (sid == 0) return false;

            // query sid and ts table.
            uint32_t sid_sel = sid;
            DB_TIMESTAMP t_latest;
            db.query_latest_ts(sid, t_latest);
            if (ts_newest && *ts_newest > t_latest) {
                t_latest = ts_newest;
            }

            if (t_latest) {
                TWESYS::TweLocalTime t;
                t.set_epoch(*t_latest);

                year = t.year;
                month = t.month;
                day = t.day;
                hour = 0;

                return true;
            }

            return false;
        }
        
        bool has_set_day() {
            if (year != 0 && month != 0 && day != 0) return true;
            else return false;
        }

        _NODE_INFO(SCR_WSNS_DB& base) : base(base), sid(0)
            , year(0), month(0), day(0), hour(0)
            , ts_oldest(DB_NULL), ts_newest(DB_NULL)
            , b_live_update(false), live_history(1024)
            {}
    } _node;

    /**
     * manage/control view of graph.
     */
    struct _VIEW {

    public:
        static const int32_t DRAW_WIDTH = 450;       // screen draw width (fixed, 3600/8)
        static const int32_t LIVE_VIEW_DUR = 450;    // Live view duration (= DRAW_WIDTH)
        static const int32_t DATA_WIDTH = 3600 * 24; // full width of data (store 1 sec step)

        // color tables
        static const uint16_t col_gray30 = color565(255 * 30 / 100, 255 * 30 / 100, 255 * 30 / 100);
        static const uint16_t col_gray80 = color565(255 * 80 / 100, 255 * 80 / 100, 255 * 80 / 100);
        static const uint16_t col_gray90 = color565(255 * 90 / 100, 255 * 90 / 100, 255 * 90 / 100);
        static const uint16_t col_white = ALMOST_WHITE;

        static const uint16_t col_darkR = RED;
        static const uint16_t col_darkG = color565(40, 160, 40);
        static const uint16_t col_darkB = BLUE;
        static const uint16_t col_LightR = color565(255, 160, 160);
        static const uint16_t col_LightG = color565(160, 255, 160);
        static const uint16_t col_LightB = color565(160, 160, 255);

        static const uint16_t col_void = color565(255, 224, 224); // for dummy sample

        // bitmap if data is stored or not.
        static const unsigned BM_VALUE = 1;
        static const unsigned BM_VALUE1 = 2;
        static const unsigned BM_VALUE2 = 4;
        static const unsigned BM_VALUE3 = 8;
        static const unsigned BM_VCC = 0x10;
        static const unsigned BM_MAG = 0x20;
        static const unsigned BM_EV_ID = 0x40;

    public:
        /**
         * clear v_dat[] arrays.
         * - intended to call before drawing newly or changing scale.
         */
        void _clear() {
            v_dat.clear();
            //v_dat.reserve(pseudo_width); // note: might have problem on SimpleBuffer<>
            v_dat.redim(pseudo_width);
            v_ave.clear();
            //v_ave.reserve(pseudo_width); // note: might have problem on SimpleBuffer<>
            v_ave.redim(pseudo_width);
            ct_plot_points = 0;

            data_type = uint8_t(E_PAL_DATA_TYPE::NODEF);
        }

        /**
         * clear _v_dat_full.
         * - intended to call before loading newly.
         */
        void clear_full() {
            v_dat_full.clear();
            v_dat_full.reserve(DATA_WIDTH);
            v_dat_full.redim(DATA_WIDTH);

            full_cursor_idx = -1;
        }

        /**
         * Add an sensor data entry into full width buffer (v_dat_full[] of DATA_WIDTH length.).
         * Note: The sensor data is stored into v_dat_full[].
         *       The vector index is split by small time slice (t_end-t_start)/DATA_WIDTH.
         *       If two or more data has same index, the last data is stored.
         *
         * \param d
         */
        void add_entry_full(WSnsDb::SENSOR_DATA& d) {
            uint64_t t = *d.ts;

            if (t >= t_start && t < t_end) {
                int t_rel = int(*d.ts - t_start);
                int i = int(t_rel / t_step_full);

                if (i >= 0 && i < DATA_WIDTH) {
                    uint64_t t = *d.ts;

                    if (t >= t_start && t < t_end) {
                        int t_rel = int(*d.ts - t_start);
                        int i = int(t_rel / t_step_full);

                        if (i >= 0 && i < DATA_WIDTH) {
                            auto& x = v_dat_full[i];
                            x = d;
                        }
                    }
                }
            }
        }

        /**
         * Add an sensor data entry into drawing width buffer (v_dat[] of DRAW_WIDTH length.).
         * Note: if the two or more entry are within the same time range, they will be averaged.
         *
         * - before calling, t_start and t_step shall be set.
         *
         * \param d    a sensor data entry.
         */
        int add_entry(WSnsDb::SENSOR_DATA& d) {
            uint64_t t = *d.ts;

            int idx_stored = -1;

            if (t >= t_start && t < t_end) {
                int t_rel = int(*d.ts - t_start);
                int i = int(t_rel / t_step);

                idx_stored = i;

                if (i >= 0 && i < pseudo_width) {
                    auto& x = v_dat[i];

                    if (!x.sid || *x.sid == 0) {
                        x = d; // COPY THE FIRST ONE
                    }
                    else {
                        #define ADD_ENTRY_ADD(N) if (d.N) { if (x.N) *x.N += *d.N; else x.N = d.N; }
                        ADD_ENTRY_ADD(value);
                        ADD_ENTRY_ADD(value1);
                        ADD_ENTRY_ADD(value2);
                        ADD_ENTRY_ADD(value3);
                        ADD_ENTRY_ADD(val_vcc_mv);
                        ADD_ENTRY_ADD(lqi);
                    }

                    // set data existence bitmap
                    if (d.value)  bm_value |= BM_VALUE;
                    if (d.value1) bm_value |= BM_VALUE1;
                    if (d.value2) bm_value |= BM_VALUE2;
                    if (d.value3) bm_value |= BM_VALUE3;
                    if (d.val_vcc_mv) bm_value |= BM_VCC;
                    if (d.val_dio && (*x.val_dio & 0x10000000)) bm_value |= BM_MAG;
                    if (d.ev_id) bm_value |= BM_EV_ID;

                    // determine datatype
                    if (data_type == uint8_t(E_PAL_DATA_TYPE::NODEF)
                        && (d.pkt_type && *d.pkt_type != uint8_t(E_PAL_DATA_TYPE::NODEF))) {
                        data_type = *d.pkt_type;
                    }

                    ++v_ave[i];
                }
            }
            return idx_stored;
        }

        /**
         * averaging entry in v_dat[].
         *
         */
        void finalize_entry() {
            int i_first_entry = -1;
            int i_last_entry = -1;

            ct_plot_points = 0;

            // do average sensor value
            for (int i = 0; i < pseudo_width; i++) {
                auto& ct = v_ave[i];

                if (ct > 0) {
                    if (i_first_entry == -1) i_first_entry = i;
                    i_last_entry = i;
                    ct_plot_points++;

                    auto& x = v_dat[i];
                    if (x.value) *x.value /= ct;
                    if (x.value1) *x.value1 /= ct;
                    if (x.value2) *x.value2 /= ct;
                    if (x.value3) *x.value3 /= ct;
                    if (x.val_vcc_mv) *x.val_vcc_mv /= ct;
                    if (x.lqi) *x.lqi /= ct;
                }
            }
        }

        /**
         * set starting epoch and graph width in time scale.
         *
         * \param start         start epoch of draw area
         * \param duration      duration[s] in draw area
         */
        void set_start_epoch_and_duration(uint64_t start, int duration, int pseudo_width_ = DRAW_WIDTH) {
            // recalc start index
            if (pseudo_scale > 1) {
                int i = pseudo_start_idx + DRAW_WIDTH / 2;
                int i_new = i * pseudo_width_ / pseudo_width;
                i_new -= DRAW_WIDTH / 2;
                if (i_new < 0) i_new = 0;
                if (i_new > pseudo_width_ - DRAW_WIDTH) i_new = pseudo_width - DRAW_WIDTH;
            }
            else {
                pseudo_start_idx = 0;
            }

            // set new width
            pseudo_width = pseudo_width_;

            // reset cursor
            pseudo_cursor_idx = -1;

            // set time relate parameters
            t_start = start;
            t_end = start + duration; // note: t_start <= t < t_end (t: timestamp of sensor node)
            t_step = duration / pseudo_width; // note: so far, duration shall be divisible by DRAW_WIDTH. (e.g. integer multiple of 3600)
            t_step_full = duration / DATA_WIDTH; // also shall be divisible by DATA_WIDTH. (24*450=10800=3H)

            _clear();

            // put ts into v_dat[], however it's not used.
            for (int i = 0; i <= pseudo_width; i++) {
                v_dat[i].ts = t_start + i * t_step; // save starting epoch of each pixel.
                v_ave[i] = 0; // set count as 0
            }

            // clear bitmap
            bm_value = 0;
        }

        /**
         * set drawing area.
         *
         * \param a     plotting area (whole)
         */
        void set_render_area(Rect a) {
            area_draw = a;
            auto& g = area_graph;
            auto& h = area_graph_sub;
            auto& s = area_scroll_bar;

            g.x = a.x + a.w - 450 - 4; // a.x + (a.w - 450) / 2;
            g.y = a.y + 14;
            g.w = 450;
            g.h = 244;

            h.x = g.x;
            h.y = area_graph.y + area_graph.h + 14;
            h.w = g.w;
            h.h = 104;

            // scroll bar
            s.x = g.x - 1;
            s.w = g.w + 2;
            s.y = h.y + h.h + 2 + 12;
            s.h = 10;

            scrbar.set_draw_area(s);
        }

        /**
         * plot a graph line <template functions>
         *
         * \param area      area to draw
         * \param colHvy    dark color (main pixels)
         * \param colLgt    light color (complement pixels)
         * \param SCALE     scaling function object.
         * \param VALUE     value extractor from WSnsDb::SENSOR_DATA
         */
        template <typename T, typename TF_SCALE, typename TF_VALUE>
        void plot_single_line(
            Rect& area,
            const uint16_t colHvy,
            const uint16_t colLgt,
            TF_SCALE&& SCALE, // std::function<int(T)> SCALE, /* note: std::function is easy to understand, but a bit inefficient */
            TF_VALUE&& VALUE  // std::function<std::optional<T>(WSnsDb::SENSOR_DATA&)> VALUE
        ) {
            auto& rndr = M5.Lcd;

            int y_last = 0; // last y pixel position
            int i_last = 0; // last index of existing sensor data

            // plot v_dat[0]
            bool b_has_valid_data_on_the_left = false;
            if (VALUE(v_dat[pseudo_start_idx])) {
                y_last = SCALE(*VALUE(v_dat[pseudo_start_idx]));
                b_has_valid_data_on_the_left = true;
            }
            else y_last = -1;

            for (int i = 1; i < DRAW_WIDTH; i++) {
                int ip = pseudo_start_idx + i;
                bool b_plotted = false;
                if (v_ave[ip] != 0) {
                    auto& x = v_dat[ip];
                    auto& x_last = v_dat[i_last + pseudo_start_idx];

                    if (VALUE(x)) {
                        // compute Y-axis
                        int y = SCALE(*VALUE(x));

                        // if last value is not exist, use current one.
                        if (!VALUE(x_last) || y_last == -1) {
                            y_last = y;
                        }

                        // draw line from last point to the current.
                        rndr.drawLine(
                            area.x + i_last, area.y + y_last,
                            area.x + i, area.y + y, colLgt
                        );

                        // plot darker pixel at the sensor value.
                        if (i_last > 1 || (i == 1 && b_has_valid_data_on_the_left)) {
                            rndr.drawPixel(area.x + i_last, area.y + y_last, v_ave[i_last + pseudo_start_idx] == -1 ? colLgt : colHvy);
                        }
                        rndr.drawPixel(area.x + i, area.y + y, v_ave[ip] == -1 ? colLgt : colHvy);

                        // plot pixels above/below at plot point (to make line thicker)
                        if (v_ave[ip] != -1 && i != 0 && i != DRAW_WIDTH - 1) {
                            rndr.drawPixel(area.x + i, area.y + y - 1, colLgt);
                            rndr.drawPixel(area.x + i, area.y + y + 1, colLgt);
                        }

                        // store the current value for next iteration
                        y_last = y;
                        i_last = i;
                        b_plotted = true;
                    }
                }

                // draw to right edge if no data there.
                if (i == DRAW_WIDTH - 1 && !b_plotted && y_last != -1) {
                    rndr.drawLine(
                        area.x + i_last + 1, area.y + y_last,
                        area.x + i, area.y + y_last, colLgt
                    );
                }
            }
        }

        void plot_cursor_control(int _new) {
            if (b_cursor_show) {
                // showing now
                if (_new == 0) {
                    screen_hide_cursor();
                    b_cursor_show = false;
                }
            }
            else {
                // hiding now
                if (_new == 1) {
                    screen_show_cursor();
                    b_cursor_show = true;
                }
            }
        }

        /**
         * plot backgrounds and graph frames, etc.
         *
         */
        void _plot_bgs(bool b_preview = false) {
            const auto& font = TWEFONT::queryFont(base._app.font_IDs.smaller);
            auto font_w = font.get_width();
            auto font_h = font.get_height();

            auto& rndr = M5.Lcd;

            rndr.fillRect(area_draw.x, area_draw.y, area_draw.w, area_draw.h, b_preview ? col_gray90 : col_white);

            if (b_preview == false) {
                const int r = 16;
                if (x_ptr != 0 && y_ptr != 0
                    && x_ptr > area_draw.x + r && x_ptr <= area_draw.x + area_draw.w - r
                    && y_ptr > area_draw.y + r && y_ptr <= area_draw.y + area_draw.h - r
                    ) {
                    rndr.fillCircle(x_ptr, y_ptr, r - 1, color565(255, 192, 192));
                    rndr.fillCircle(x_ptr, y_ptr, r - 3, color565(255, 224, 224));
                    rndr.fillCircle(x_ptr, y_ptr, r - 6, color565(255, 240, 240));

                    plot_cursor_control(0); // hide
                }
                else {
                    plot_cursor_control(1); // show
                }
            }
            else {
                plot_cursor_control(1); // show
            }

            rndr.drawRect(area_graph.x - 1, area_graph.y - 1, area_graph.w + 2, area_graph.h + 2, col_gray30);
            rndr.drawRect(area_graph_sub.x - 1, area_graph_sub.y - 1, area_graph_sub.w + 2, area_graph_sub.h + 2, col_gray30);

            if (b_preview) {
                const wchar_t* msg = L"-- プレビュー --";
                int l_msg = TWEUTILS::strlen_vis(msg);

                drawChar(font
                    , area_draw.x + area_draw.w / 2 - font_w * l_msg / 2
                    , area_draw.y + area_draw.h / 2 - font_h / 2
                    , msg, col_gray30, WHITE, 0, M5);
            }
            else {
                if (v_dat[pseudo_start_idx].ts) {
                    auto t0 = t_start + pseudo_start_idx * t_step;
                    TWESYS::TweLocalTime tl_0, tl_1, tl_m;
                    tl_0.set_epoch(t0);
                    tl_1.set_epoch(t0 + (DRAW_WIDTH)*t_step - 1);
                    tl_m.set_epoch(t0 + (DRAW_WIDTH / 2) * t_step);

                    SmplBuf_ByteSL<64> lbl0, lbl1, lblm;
                    lbl0 << format("%02d:%02d:%02d", tl_0.hour, tl_0.minute, tl_0.second);
                    lbl1 << format("%02d:%02d:%02d", tl_1.hour, tl_1.minute, tl_1.second);
                    lblm << format("%02d:%02d:%02d", tl_m.hour, tl_m.minute, tl_m.second);

                    int32_t y_time_label = area_graph_sub.y + area_graph_sub.h + 1;

                    drawChar(font
                        , area_graph.x
                        , y_time_label
                        , lbl0.c_str(), col_gray30, WHITE, 0, M5);
                    drawChar(font
                        , area_graph.x + area_graph.w - font_w * 8
                        , y_time_label
                        , lbl1.c_str(), col_gray30, WHITE, 0, M5);
                    drawChar(font
                        , area_graph.x + area_graph.w / 2 - font_w * 4
                        , y_time_label
                        , lblm.c_str(), col_gray30, WHITE, 0, M5);
                }

                for (int i = 1; i <= 5; i++) {
                    rndr.drawLine(
                        area_graph.x + 75 * i, area_graph.y,
                        area_graph.x + 75 * i, area_graph.y + area_graph.h - 1,
                        col_gray80
                    );

                    rndr.drawLine(
                        area_graph_sub.x + 75 * i, area_graph_sub.y,
                        area_graph_sub.x + 75 * i, area_graph_sub.y + area_graph_sub.h - 1,
                        col_gray80
                    );
                }

                if (pseudo_cursor_idx != -1) {
                    int x = pseudo_cursor_idx - pseudo_start_idx;
                    if (x >= 0 && x < DRAW_WIDTH) {
                        rndr.drawLine(
                            area_graph.x + x, area_graph.y,
                            area_graph.x + x, area_graph.y + area_graph.h - 1,
                            MAGENTA
                        );

                        rndr.drawLine(
                            area_graph_sub.x + x, area_graph_sub.y,
                            area_graph_sub.x + x, area_graph_sub.y + area_graph_sub.h - 1,
                            MAGENTA
                        );
                    }
                }

                // main part
                {
                    int x_pos = area_graph.x + area_graph.w - 1;
                    int y_pos = area_graph.y - font_h - 1;

                    if (IS_PKT_TYPE_ARIA_AMB(data_type)) {
                        if (bm_value & BM_VALUE2) {
                            const wchar_t* lbl = L"照度[lx]";
                            x_pos -= strlen_vis(lbl) * font_w;
                            drawChar(font
                                , x_pos, y_pos
                                , lbl, col_darkG, WHITE, 0, M5);

                            x_pos -= font_w;
                        }

                        if (bm_value & BM_VALUE1) {
                            const wchar_t* lbl = L"湿度[％]";
                            x_pos -= strlen_vis(lbl) * font_w;
                            drawChar(font
                                , x_pos, y_pos
                                , lbl, col_darkB, WHITE, 0, M5);

                            x_pos -= font_w;
                        }

                        if (bm_value & BM_VALUE) {
                            const wchar_t* lbl = L"温度[℃]";
                            x_pos -= strlen_vis(lbl) * font_w;
                            drawChar(font
                                , x_pos, y_pos
                                , lbl, col_darkR, WHITE, 0, M5);

                            x_pos -= font_w;
                        }
                    }
                    else if (IS_PKT_APPTWELITE(data_type)) {
                        if (bm_value & BM_VALUE3) {
                            const wchar_t* lbl = L"ADC3[V]";
                            x_pos -= strlen_vis(lbl) * font_w;
                            drawChar(font
                                , x_pos, y_pos
                                , lbl, col_gray30, WHITE, 0, M5);

                            x_pos -= font_w;
                        }

                        if (bm_value & BM_VALUE2) {
                            const wchar_t* lbl = L"ADC2[V]";
                            x_pos -= strlen_vis(lbl) * font_w;
                            drawChar(font
                                , x_pos, y_pos
                                , lbl, col_darkG, WHITE, 0, M5);

                            x_pos -= font_w;
                        }

                        if (bm_value & BM_VALUE1) {
                            const wchar_t* lbl = L"ADC1[V]";
                            x_pos -= strlen_vis(lbl) * font_w;
                            drawChar(font
                                , x_pos, y_pos
                                , lbl, col_darkB, WHITE, 0, M5);

                            x_pos -= font_w;
                        }

                        if (bm_value & BM_VALUE) {
                            const wchar_t* lbl = L"DIO";
                            x_pos -= strlen_vis(lbl) * font_w;
                            drawChar(font
                                , x_pos, y_pos
                                , lbl, col_darkR, WHITE, 0, M5);

                            x_pos -= font_w;
                        }
                    }
                    else if (IS_PKT_TYPE_MOT_CUE(data_type)) {
                        if (bm_value & BM_EV_ID) {
                            const wchar_t* lbl = L"EVENT";
                            x_pos -= strlen_vis(lbl) * font_w;
                            drawChar(font
                                , x_pos, y_pos
                                , lbl, col_gray30, WHITE, 0, M5);

                            x_pos -= font_w;
                        }
                        
                        if (bm_value & BM_VALUE2) {
                            const wchar_t* lbl = L"Z[G]";
                            x_pos -= strlen_vis(lbl) * font_w;
                            drawChar(font
                                , x_pos, y_pos
                                , lbl, col_darkB, WHITE, 0, M5);

                            x_pos -= font_w;
                        }

                        if (bm_value & BM_VALUE1) {
                            const wchar_t* lbl = L"Y[G]";
                            x_pos -= strlen_vis(lbl) * font_w;
                            drawChar(font
                                , x_pos, y_pos
                                , lbl, col_darkG, WHITE, 0, M5);

                            x_pos -= font_w;
                        }

                        if (bm_value & BM_VALUE) {
                            const wchar_t* lbl = L"X[G]";
                            x_pos -= strlen_vis(lbl) * font_w;
                            drawChar(font
                                , x_pos, y_pos
                                , lbl, col_darkR, WHITE, 0, M5);

                            x_pos -= font_w;
                        }
                    }
                    else if (IS_PKT_MAG(data_type)) {
                        if (bm_value & BM_VALUE) {
                            const wchar_t* lbl2 = L"2:S極";
                            x_pos -= strlen_vis(lbl2) * font_w;
                            drawChar(font, x_pos, y_pos, lbl2, col_darkB, WHITE, 0, M5);
                            x_pos -= font_w;

                            const wchar_t* lbl1 = L"1:N極";
                            x_pos -= strlen_vis(lbl1) * font_w;
                            drawChar(font, x_pos, y_pos, lbl1, col_darkR, WHITE, 0, M5);
                            x_pos -= font_w;

                            const wchar_t* lbl0 = L"0:なし";
                            x_pos -= strlen_vis(lbl0) * font_w;
                            drawChar(font, x_pos, y_pos, lbl0, col_gray30, WHITE, 0, M5);
                            x_pos -= font_w;

                            const wchar_t* lbl = L"磁石";
                            x_pos -= strlen_vis(lbl) * font_w;
                            drawChar(font, x_pos, y_pos, lbl, col_darkG, WHITE, 0, M5);
                            x_pos -= font_w;
                        }
                    }
                }

                for (int i = 0; i <= 10; i++) {
                    int y = area_graph.y + 2 + i * (area_graph.h - 4 - 1) / 10;
                    rndr.drawLine(
                        area_graph.x, y,
                        area_graph.x + area_graph.w - 1, y,
                        col_gray80
                    );

                    if (IS_PKT_TYPE_ARIA_AMB(data_type)) {
                        switch (i) {
                        case 0: if (bm_value & BM_VALUE2) drawChar(font, area_graph.x - font_w * 4 - 2, y - font_h / 2, L"2000", col_darkG, WHITE, 0, M5); break;
                        case 10: if (bm_value & BM_VALUE2) drawChar(font, area_graph.x - font_w * 3 - 2, y - font_h / 2, L"  0", col_darkG, WHITE, 0, M5); break;
                        case 1: if (bm_value & BM_VALUE1) drawChar(font, area_graph.x - font_w * 3 - 2, y - font_h / 2, L"90%", col_darkB, WHITE, 0, M5); break;
                        case 3: if (bm_value & BM_VALUE1) drawChar(font, area_graph.x - font_w * 3 - 2, y - font_h / 2, L"70%", col_darkB, WHITE, 0, M5); break;
                        case 5: if (bm_value & BM_VALUE1) drawChar(font, area_graph.x - font_w * 3 - 2, y - font_h / 2, L"50%", col_darkB, WHITE, 0, M5); break;
                        case 7: if (bm_value & BM_VALUE1) drawChar(font, area_graph.x - font_w * 3 - 2, y - font_h / 2, L"30%", col_darkB, WHITE, 0, M5); break;
                        case 9: if (bm_value & BM_VALUE1) drawChar(font, area_graph.x - font_w * 3 - 2, y - font_h / 2, L"10%", col_darkB, WHITE, 0, M5); break;
                        case 2: if (bm_value & BM_VALUE) drawChar(font, area_graph.x - font_w * 4 - 2, y - font_h / 2, L"30℃", col_darkR, WHITE, 0, M5); break;
                        case 4: if (bm_value & BM_VALUE) drawChar(font, area_graph.x - font_w * 4 - 2, y - font_h / 2, L"20℃", col_darkR, WHITE, 0, M5); break;
                        case 6: if (bm_value & BM_VALUE) drawChar(font, area_graph.x - font_w * 4 - 2, y - font_h / 2, L"10℃", col_darkR, WHITE, 0, M5); break;
                        case 8: if (bm_value & BM_VALUE) drawChar(font, area_graph.x - font_w * 4 - 2, y - font_h / 2, L" 0℃", col_darkR, WHITE, 0, M5); break;
                        }
                    }
                    else if (IS_PKT_APPTWELITE(data_type)) {
                        switch (i) {
                        case 4: drawChar(font, area_graph.x - font_w * 4 - 2, y - font_h / 2, L"  3V", col_gray30, WHITE, 0, M5); break;
                        case 6: drawChar(font, area_graph.x - font_w * 4 - 2, y - font_h / 2, L"  2V", col_gray30, WHITE, 0, M5); break;
                        case 8: drawChar(font, area_graph.x - font_w * 4 - 2, y - font_h / 2, L"  1V", col_gray30, WHITE, 0, M5); break;
                        case 10: drawChar(font, area_graph.x - font_w * 4 - 2, y - font_h / 2, L"  0V", col_gray30, WHITE, 0, M5); break;
                        }
                    }
                    else if (IS_PKT_TYPE_MOT_CUE(data_type)) {
                        switch (i) {
                        case 1: drawChar(font, area_graph.x - font_w * 3 - 2, y - font_h / 2, L" 2G", col_darkB, WHITE, 0, M5); break;
                        case 3: drawChar(font, area_graph.x - font_w * 3 - 2, y - font_h / 2, L" 1G", col_darkB, WHITE, 0, M5); break;
                        case 5: drawChar(font, area_graph.x - font_w * 3 - 2, y - font_h / 2, L" 0G", col_darkB, WHITE, 0, M5); break;
                        case 7: drawChar(font, area_graph.x - font_w * 3 - 2, y - font_h / 2, L"-1G", col_darkB, WHITE, 0, M5); break;
                        case 9: drawChar(font, area_graph.x - font_w * 3 - 2, y - font_h / 2, L"-2G", col_darkB, WHITE, 0, M5); break;
                        }
                    }
                    else if (IS_PKT_MAG(data_type)) {
                        switch (i) {
                        case 1: drawChar(font, area_graph.x - font_w * 4 - 2, y - font_h / 2, L"Ｓ極", col_darkB, WHITE, 0, M5); break;
                        case 5: drawChar(font, area_graph.x - font_w * 4 - 2, y - font_h / 2, L"Ｎ極", col_darkR, WHITE, 0, M5); break; 
                        case 9: drawChar(font, area_graph.x - font_w * 4 - 2, y - font_h / 2, L"なし", col_gray30, WHITE, 0, M5); break;
                        }
                    }
                }

                // sub part
                for (int i = 0; i <= 4; i++) {
                    int y = area_graph_sub.y + 2 + i * (area_graph_sub.h - 4 - 1) / 4;
                    rndr.drawLine(
                        area_graph_sub.x, y,
                        area_graph_sub.x + area_graph_sub.w - 1, y,
                        col_gray80
                    );

                    switch (i) {
                    case 1: drawChar(font, area_graph.x - font_w * 3 - 2, y - font_h / 2, L"150", col_gray30, WHITE, 0, M5); break;
                    case 2: drawChar(font, area_graph.x - font_w * 3 - 2, y - font_h / 2, L"100", col_gray30, WHITE, 0, M5); break;
                    case 3: drawChar(font, area_graph.x - font_w * 3 - 2, y - font_h / 2, L" 50", col_gray30, WHITE, 0, M5); break;
                    case 0: drawChar(font, area_graph.x - font_w * 4 - 2, y - font_h / 2, L"3.6V", col_darkR, WHITE, 0, M5); break;
                    case 4: drawChar(font, area_graph.x - font_w * 4 - 2, y - font_h / 2, L"2.0V", col_darkR, WHITE, 0, M5); break;
                    }
                }

                {
                    int x_pos = area_graph_sub.x + area_graph_sub.w - 1;
                    int y_pos = area_graph_sub.y - font_h - 1;

                    if (bm_value & BM_MAG) {
                        const wchar_t* lbl_mag = L"磁石";
                        x_pos -= strlen_vis(lbl_mag) * font_w;
                        drawChar(font
                            , x_pos
                            , y_pos
                            , lbl_mag, col_darkG, WHITE, 0, M5);

                        x_pos -= font_w;
                    }

                    const wchar_t* lbl_lqi = L"LQI";
                    x_pos -= strlen_vis(lbl_lqi) * font_w;
                    drawChar(font
                        , x_pos
                        , y_pos
                        , lbl_lqi, col_gray30, WHITE, 0, M5);

                    x_pos -= font_w;

                    if (bm_value & BM_VCC) {
                        const wchar_t* lbl_vcc = L"電源[V]";
                        x_pos -= strlen_vis(lbl_vcc) * font_w;
                        drawChar(font
                            , x_pos
                            , y_pos
                            , lbl_vcc, col_darkR, WHITE, 0, M5);
                    }
                }

            }
        }

        /**
         * show no data in plot area.
         *
         */
        void plot_empty(bool b_preview = false) {
            auto& rndr = M5.Lcd;

            const auto& font = TWEFONT::queryFont(base._app.font_IDs.smaller);
            auto font_w = font.get_width();
            auto font_h = font.get_height();

            rndr.fillRect(area_draw.x, area_draw.y, area_draw.w, area_draw.h, b_preview ? col_gray90 : col_white);

            if (ct_plot_points == 0) {
                const wchar_t* msg = L"表示データが存在しないか、この画面では表示を行いません";
                drawChar(font
                    , area_graph.x + font_w * 2
                    , area_graph.y + area_graph.h / 2 - font_h / 2
                    , msg, col_gray30, WHITE, 0, M5);
                return;
            }
        }

        /**
         * save mouse cursor position.
         * 
         * \param x
         * \param y
         */
        void set_pointer_pos(int x, int y) {
            x_ptr = x;
            y_ptr = y;
        }

        /**
         * plot the data.
         * - before plot, add_entry() and finalize_entry() shall be performed.
         *
         */
        void plot_graph(bool b_preview = false) {
            auto& rndr = M5.Lcd;

            const auto& font = TWEFONT::queryFont(base._app.font_IDs.smaller);
            auto font_w = font.get_width();
            auto font_h = font.get_height();


            // clear bg
            if (ct_plot_points == 0) {
                plot_empty(b_preview);
                return;
            }
            else {
                // update data type.
                if (pseudo_start_idx != -1) {
                    for (int i = pseudo_start_idx; i < pseudo_start_idx + DRAW_WIDTH; i++) {
                        if (v_dat[i].sid && *v_dat[i].sid != 0) {
                            data_type = *v_dat[i].pkt_type; // determine by the first sample of `.pkt_type'.
                            break;
                        }
                    }
                }

                _plot_bgs(b_preview);
            }


            // scaler
            struct _SCALE_Y {
                int inv(int y) { return (y_scale - 1) - y + MARGIN; } // 2px margin at top and bottom
                int y_temp(double v) {
                    int y = int((y_scale - 1) * (v + 10.) / 50.);
                    if (y <= 0) y = 0;
                    if (y >= y_scale - 1) y = y_scale - 1;
                    return inv(y);
                }
                int y_humd(double v) {
                    int y = int((y_scale - 1) * v / 100.);
                    if (y <= 0) y = 0;
                    if (y >= y_scale - 1) y = y_scale - 1;
                    return inv(y);
                }
                int y_lumi(double v) {
                    int y = int((y_scale - 1) * v / 2000.);
                    if (y <= 0) y = 0;
                    if (y >= y_scale - 1) y = y_scale - 1;
                    return inv(y);
                }
                int y_vcc(int32_t v) {
                    int y = (y_scale - 1) * (v - 2000) / 1600;
                    if (y <= 0) y = 0;
                    if (y >= y_scale - 1) y = y_scale - 1;
                    return inv(y);
                }
                int y_lqi(int32_t v) {
                    int y = (y_scale - 1) * v / 200;
                    if (y <= 0) y = 0;
                    if (y >= y_scale - 1) y = y_scale - 1;
                    return inv(y);
                }

                int y_dio_4p(double v) {
                    const unsigned tbl[16] = {
                        0b1111, // 0000
                        0b0111, // 0001
                        0b1011, // 0010
                        0b0011, // 0011
                        0b1101, // 0100
                        0b0101, // 0101
                        0b1001, // 0110
                        0b0001, // 0111
                        0b1110, // 1000
                        0b0110, // 1001
                        0b1010, // 1010
                        0b0010, // 1011
                        0b1100, // 1100
                        0b0100, // 1101
                        0b1000, // 1110
                        0b0000 // 1111
                    };
                    int n = int(v);
                    if (n < 0) n = 0;
                    if (n > 15) n = 15;
                    n = tbl[n];

                    int y = int((y_scale - 1) * n / 20);
                    if (y <= 0) y = 0;
                    if (y >= y_scale - 1) y = y_scale - 1;
                    return inv(y);
                }

                int y_adc_5v(double v) {
                    int y = int((y_scale - 1) * v / 5.0);
                    if (y <= 0) y = 0;
                    if (y >= y_scale - 1) y = y_scale - 1;
                    return inv(y);
                }

                int y_accel(double v) {
                    int y = int((y_scale - 1) * (v + 2.5) / 5.0);
                    if (y <= 0) y = 0;
                    if (y >= y_scale - 1) y = y_scale - 1;
                    return inv(y);
                }

                int y_accel_ev(int32_t v) {
                    int y = (y_scale - 1) * v / 20;
                    if (y <= 0) y = 0;
                    if (y >= y_scale - 1) y = y_scale - 1;
                    return inv(y);
                }

                int y_mag(int32_t v) {
                    int y = (y_scale - 1) * (v * 40 + 10) / 100;
                    if (y <= 0) y = 0;
                    if (y >= y_scale - 1) y = y_scale - 1;
                    return inv(y);
                }

                const int MARGIN; // marin pixel bottom/top.
                int y_scale;

                void set_scale(int sc) { y_scale = sc - MARGIN * 2; }

                _SCALE_Y() : MARGIN(2), y_scale(100) {}
            } SCALE_Y;

            // plot onto main area
            SCALE_Y.set_scale(area_graph.h);

            // plot ambient type (temperature, humidity, luminance)
            if (IS_PKT_TYPE_ARIA_AMB(data_type)) {
                plot_single_line<DB_REAL::value_type>(area_graph, col_darkR, col_LightR,
                    [&SCALE_Y](DB_REAL::value_type x) -> int { return SCALE_Y.y_temp(x); },
                    [](WSnsDb::SENSOR_DATA& d) -> DB_REAL { return d.value; }
                );

                plot_single_line<DB_REAL::value_type>(area_graph, col_darkB, col_LightB,
                    [&SCALE_Y](DB_REAL::value_type x) -> int { return SCALE_Y.y_humd(x); },
                    [](WSnsDb::SENSOR_DATA& d) -> DB_REAL { return d.value1; }
                );

                if (bm_value & BM_VALUE2) {
                    plot_single_line<DB_REAL::value_type>(area_graph, col_darkG, col_LightG,
                        [&SCALE_Y](DB_REAL::value_type x) -> int { return SCALE_Y.y_lumi(x); },
                        [](WSnsDb::SENSOR_DATA& d) -> DB_REAL { return d.value2; }
                    );
                }

                //plot_single_line<DB_REAL::value_type>(area_graph, col_gray30, col_gray80,
                //    [&SCALE_Y](DB_REAL::value_type x) -> int { return SCALE_Y.y_lumi(x); },
                //    [](WSnsDb::SENSOR_DATA& d) -> DB_REAL { return d.value3; }
                //);
            }
            else if (IS_PKT_APPTWELITE(data_type)) {
                plot_single_line<DB_REAL::value_type>(area_graph, col_darkR, col_LightR,
                    [&SCALE_Y](DB_REAL::value_type x) -> int { return SCALE_Y.y_dio_4p(x); },
                    [](WSnsDb::SENSOR_DATA& d) -> DB_REAL { return d.value; }
                );

                plot_single_line<DB_REAL::value_type>(area_graph, col_darkB, col_LightB,
                    [&SCALE_Y](DB_REAL::value_type x) -> int { return SCALE_Y.y_adc_5v(x); },
                    [](WSnsDb::SENSOR_DATA& d) -> DB_REAL { return d.value1; }
                );
            }
            else if (IS_PKT_TYPE_MOT_CUE(data_type)) {
                plot_single_line<DB_REAL::value_type>(area_graph, col_darkR, col_LightR,
                    [&SCALE_Y](DB_REAL::value_type x) -> int { return SCALE_Y.y_accel(x); },
                    [](WSnsDb::SENSOR_DATA& d) -> DB_REAL { return d.value; }
                );

                plot_single_line<DB_REAL::value_type>(area_graph, col_darkG, col_LightG,
                    [&SCALE_Y](DB_REAL::value_type x) -> int { return SCALE_Y.y_accel(x); },
                    [](WSnsDb::SENSOR_DATA& d) -> DB_REAL { return d.value1; }
                );

                plot_single_line<DB_REAL::value_type>(area_graph, col_darkB, col_LightB,
                    [&SCALE_Y](DB_REAL::value_type x) -> int { return SCALE_Y.y_accel(x); },
                    [](WSnsDb::SENSOR_DATA& d) -> DB_REAL { return d.value2; }
                );

                if (bm_value & BM_EV_ID) {
                    plot_single_line<DB_INTEGER::value_type>(area_graph, col_gray30, col_gray80,
                        [&SCALE_Y](DB_INTEGER::value_type x) -> int { return SCALE_Y.y_accel_ev(x); },
                        [](WSnsDb::SENSOR_DATA& d) -> DB_INTEGER { return d.ev_id; }
                    );
                }
            }
            else if (IS_PKT_MAG(data_type)) {
                plot_single_line<DB_INTEGER::value_type>(area_graph, col_darkG, col_LightG,
                    [&SCALE_Y](DB_INTEGER::value_type x) -> int { return SCALE_Y.y_mag(x); },
                    [](WSnsDb::SENSOR_DATA& d) -> DB_INTEGER {
                        if (d.val_dio && (*d.val_dio & 0x10000000))
                            return DB_INTEGER((*d.val_dio >> 24) & 0x3);
                        else return DB_INTEGER();
                    }
                );
            }

            // plot onto sub area for common information
            SCALE_Y.set_scale(area_graph_sub.h);
            if (bm_value & BM_VCC) {
                plot_single_line<DB_INTEGER::value_type>(area_graph_sub, col_darkR, col_LightR,
                    [&SCALE_Y](DB_INTEGER::value_type x) -> int { return SCALE_Y.y_vcc(x); },
                    [](WSnsDb::SENSOR_DATA& d) -> DB_INTEGER { return d.val_vcc_mv; }
                );
            }
            plot_single_line<DB_INTEGER::value_type>(area_graph_sub, col_gray30, col_gray80,
                [&SCALE_Y](DB_INTEGER::value_type x) -> int { return SCALE_Y.y_lqi(x); },
                [](WSnsDb::SENSOR_DATA& d) -> DB_INTEGER { return d.lqi; }
            );
            if (bm_value & BM_MAG) {
                plot_single_line<DB_INTEGER::value_type>(area_graph_sub, col_darkG, col_LightG,
                    [&SCALE_Y](DB_INTEGER::value_type x) -> int { return SCALE_Y.y_mag(x); },
                    [](WSnsDb::SENSOR_DATA& d) -> DB_INTEGER {
                        if (d.val_dio && (*d.val_dio & 0x10000000))
                            return DB_INTEGER((*d.val_dio >> 24) & 0x3);
                        else return DB_INTEGER();
                    }
                );
            }

            if (pseudo_scale != 1) {
                scrbar.set_value(pseudo_start_idx, pseudo_start_idx + DRAW_WIDTH, 0, DRAW_WIDTH * pseudo_scale);
                scrbar.update_view();
            }

            // further info.
            if (pseudo_cursor_idx != -1) {
                auto& rndr = M5.Lcd;
                auto& d = sns_pick.d;

                int area = 0;
                if (is_in_area(area_graph, x_ptr, y_ptr)) area = 1;
                if (is_in_area(area_graph_sub, x_ptr, y_ptr)) area = 2;

                const auto& font = TWEFONT::queryFont(base._app.font_IDs.tiny);
                auto font_w = font.get_width();
                auto font_h = font.get_height();

                int data_count = 0;

                if (area) {
                    if (area == 1) {
                        if (IS_PKT_TYPE_ARIA_AMB(data_type)) {
                            data_count = (!!d.value + !!d.value1 + !!d.value2 + !!d.value3);
                        }
                        else if (IS_PKT_APPTWELITE(data_type)) {
                            data_count = (!!d.value + !!d.value1 + !!d.value2 + !!d.value3);
                        }
                        else if (IS_PKT_TYPE_MOT_CUE(data_type)) {
                            data_count = (!!d.value + !!d.value1 + !!d.value2 + !!d.ev_id);
                        }
                        else if (IS_PKT_MAG(data_type)) {
                            data_count = (!!d.value);
                        }
                    }
                    if (area == 2) {
                        data_count = (!!d.lqi + !!d.val_vcc_mv);
                        if (d.val_dio && (*d.val_dio & 0x10000000)) {
                            data_count++;
                        }
                    }

                    if (data_count > 0) {
                        Rect rc;
                        SmplBuf_WCharSL<63> lbl;

                        TWESYS::TweLocalTime td;
                        td.set_epoch(*d.ts);

                        const int COLS = 10;
                        const int MARGIN = 4;

                        rc.w = font_w * COLS + MARGIN;
                        rc.h = font_h * data_count + font_h + MARGIN;

                        int x_cur = pseudo_cursor_idx - pseudo_start_idx;
                        if (x_cur >= 0 && x_cur < DRAW_WIDTH) x_cur = area_graph.x + x_cur; else x_cur = x_ptr;
                        if (x_cur < area_draw.x + area_draw.w - rc.w - 10) rc.x = x_cur + 10; else rc.x = x_cur - 10 - rc.w;
                        if (y_ptr < area_draw.y + area_draw.h - rc.h - 10) rc.y = y_ptr + 10; else rc.y = y_ptr - 10 - rc.h;

                        rndr.fillRect(rc.x, rc.y, rc.w, rc.h, col_white);
                        rndr.drawRect(rc.x, rc.y, rc.w, rc.h, MAGENTA);

                        int y_pos = rc.y + 2;

                        if (area == 1) {
                            if (IS_PKT_TYPE_ARIA_AMB(data_type)) {
                                if (d.value) {
                                    lbl.clear();
                                    lbl << L"温:" << format("%1.2f", *d.value);
                                    drawChar(font, rc.x + 2, y_pos, lbl.c_str(), col_darkR, WHITE, 0, M5);
                                    y_pos += font_h;
                                }
                                if (d.value1) {
                                    lbl.clear();
                                    lbl << L"湿:" << format("%2.1f", *d.value1);
                                    drawChar(font, rc.x + 2, y_pos, lbl.c_str(), col_darkB, WHITE, 0, M5);
                                    y_pos += font_h;
                                }
                                if (d.value2) {
                                    lbl.clear();
                                    lbl << L"照:" << format("%d", convert_double_to_int32_t(*d.value2, 0, 1000000, -1));
                                    drawChar(font, rc.x + 2, y_pos, lbl.c_str(), col_darkG, WHITE, 0, M5);
                                    y_pos += font_h;
                                }
                            }
                            else if (IS_PKT_APPTWELITE(data_type)) {
                                if (d.val_dio) {
                                    const char tbl[16][8] = {
                                        "HHHH",
                                        "LHHH",
                                        "HLHH",
                                        "LLHH",
                                        "HHLH",
                                        "LHLH",
                                        "HLLH",
                                        "LLLH",
                                        "HHHL",
                                        "LHHL",
                                        "HLHL",
                                        "LLHL",
                                        "HHLL",
                                        "LHLL",
                                        "HLLL",
                                        "LLLL",
                                    };
                                    lbl.clear();
                                    lbl << L"DI:" << tbl[*d.val_dio & 0xF];
                                    lbl[2] = L':';
                                    drawChar(font, rc.x + 2, y_pos, lbl.c_str(), col_darkR, WHITE, 0, M5);
                                    y_pos += font_h;
                                }
                                if (d.value1) {
                                    lbl.clear();
                                    lbl << L"A1:" << format("%1.3f", *d.value1);
                                    drawChar(font, rc.x + 2, y_pos, lbl.c_str(), col_darkB, WHITE, 0, M5);
                                    y_pos += font_h;
                                }

                                if (d.value2) {
                                    lbl.clear();
                                    lbl << L"A2:" << format("%1.3f", *d.value2);
                                    drawChar(font, rc.x + 2, y_pos, lbl.c_str(), col_darkG, WHITE, 0, M5);
                                    y_pos += font_h;
                                }

                                if (d.value3) {
                                    lbl.clear();
                                    lbl << L"A3:" << format("%1.3f", *d.value3);
                                    drawChar(font, rc.x + 2, y_pos, lbl.c_str(), col_gray30, WHITE, 0, M5);
                                    y_pos += font_h;
                                }
                            }
                            else if (IS_PKT_TYPE_MOT_CUE(data_type)) {
                                if (d.value) {
                                    lbl.clear();
                                    lbl << L"X:" << format("%1.3f", *d.value);
                                    drawChar(font, rc.x + 2, y_pos, lbl.c_str(), col_darkR, WHITE, 0, M5);
                                    y_pos += font_h;
                                }
                                if (d.value1) {
                                    lbl.clear();
                                    lbl << L"Y:" << format("%1.3f", *d.value1);
                                    drawChar(font, rc.x + 2, y_pos, lbl.c_str(), col_darkG, WHITE, 0, M5);
                                    y_pos += font_h;
                                }
                                if (d.value2) {
                                    lbl.clear();
                                    lbl << L"Z:" << format("%1.3f", *d.value2);
                                    drawChar(font, rc.x + 2, y_pos, lbl.c_str(), col_darkB, WHITE, 0, M5);
                                    y_pos += font_h;
                                }
                                if (d.ev_id) {
                                    lbl.clear();
                                    lbl << format("EV:%d", *d.ev_id);
                                    drawChar(font, rc.x + 2, y_pos, lbl.c_str(), col_gray30, WHITE, 0, M5);
                                    y_pos += font_h;
                                }
                            }
                            else if (IS_PKT_MAG(data_type)) {
                                if (d.val_dio && (*d.val_dio & 0x10000000)) {
                                    lbl.clear();
                                    int v_mag = (*d.val_dio >> 24) & 0x3;
                                    const wchar_t lblMag[4][4] = { L"なし", L"N", L"S", L"-" };
                                    lbl << "M:" << lblMag[v_mag]; 
                                    const uint16_t cols[4] = { col_gray30, col_darkR, col_darkB, WHITE };
                                    drawChar(font, rc.x + 2, y_pos, lbl.c_str(), cols[v_mag], WHITE, 0, M5);
                                    y_pos += font_h;
                                }
                            }
                        }
                        else if (area == 2) {
                            if (d.val_vcc_mv) {
                                lbl.clear();
                                lbl << format("V:%d.%03dV", *d.val_vcc_mv / 1000, *d.val_vcc_mv % 1000);
                                drawChar(font, rc.x + 2, y_pos, lbl.c_str(), col_darkR, WHITE, 0, M5);
                                y_pos += font_h;
                            }
                            if (d.lqi) {
                                lbl.clear();
                                lbl << format("L:%03d", *d.lqi);
                                drawChar(font, rc.x + 2, y_pos, lbl.c_str(), col_gray30, WHITE, 0, M5);
                                y_pos += font_h;
                            }
                            if (d.val_dio && (*d.val_dio & 0x10000000)) {
                                lbl.clear();
                                int v_mag = (*d.val_dio >> 24) & 0x3;
                                const wchar_t lblMag[4][4] = { L"なし", L"N", L"S", L"-" };
                                lbl << "M:" << lblMag[v_mag];
                                const uint16_t cols[4] = { col_gray30, col_darkR, col_darkB, WHITE };
                                drawChar(font, rc.x + 2, y_pos, lbl.c_str(), cols[v_mag], WHITE, 0, M5);
                                y_pos += font_h;
                            }
                        }

                        // time
                        lbl.clear(); lbl << format("<%02d:%02d:%02d>", sns_pick.lt.hour, sns_pick.lt.minute, sns_pick.lt.second);
                        drawChar(font, rc.x + 2, y_pos, lbl.c_str(), col_gray30, WHITE, 0, M5);
                        y_pos += font_h;
                    }
                }
            }
        }

        /**
         * Preview of 1day graph.
         * 
         * \param sid
         * \param year      
         * \param month
         * \param day       
         * \param dir       -1: seek previous day with data, 1: seek next day with data, 0: find the day.
         */
        void plot_day_preview(uint32_t sid, int16_t year, int16_t month, int16_t day, int16_t dir = 0) {
            WSnsDb& db = *base._db;
            //auto& node = base._node;
            auto& view = base._view;

            TWESYS::TweLocalTime t;
            if (year != 0) {
                t.year = year;
                t.month = month;
                t.day = day;
                t.hour = 0;
                t.minute = 0;
                t.second = 0;
                t.get_epoch();
            }
            else {
                t.now();
            }

            // check latest ts, if no data in today, try to find older data.
            DB_TIMESTAMP ts_latest;
            db.query_latest_ts(sid, ts_latest);

            if (ts_latest && uint64_t(*ts_latest) < t.epoch) {
                t.set_epoch(*ts_latest);
                t.hour = 0;
                t.minute = 0;
                t.second = 0;
                t.get_epoch();
            }

            // save preview date
            sid_preview = sid;
            lt_preview = t;

            // reset the position
            view.full_cursor_idx = -1;
            view.pseudo_cursor_idx = -1;
            view.pseudo_scale = 1;
            view.set_start_epoch_and_duration(t.epoch, 86400);

            uint32_t ct = 0, t0 = millis();
            // db.query_sensor_data_by_day(node.sid, node.year, node.month, node.day, 
            db.query_sensor_data(sid, t.epoch, t.epoch + 86399,
                [&](WSnsDb::SENSOR_DATA& d) {
                    base._view.add_entry(d);
                    ct++;
                },
                2 // random, limit query count
            );
            uint32_t t1 = millis();
            base.the_screen_b << crlf << format("<%d:Q_day=%d ct=%d t=%d>", t1, day, ct, t1 - t0);

            view.finalize_entry();
            
            view.plot_graph(true);

            // draw date string
            {
                const auto& font = TWEFONT::queryFont(base._app.font_IDs.smaller);
                auto font_w = font.get_width();
                auto font_h = font.get_height();

                if (ct > 0) {
                    //int x_pos = area_graph.x + area_graph.w - 1;
                    // int y_pos = area_graph.y - font_h - 1;
                    int x_pos = area_graph.x + area_graph.w / 2;
                    int y_pos = area_graph.y + area_graph.h / 2;

                    SmplBuf_WCharS lbl;
                    lbl << format("%04d/%02d/%02d", t.year, t.month, t.day);

                    // x_pos -= TWEUTILS::strlen_vis(lbl.c_str()) * font_w;
                    x_pos -= TWEUTILS::strlen_vis(lbl.c_str()) * font_w / 2;
                    drawChar(font
                        , x_pos, y_pos
                        , lbl.c_str(), col_darkG, WHITE, 0, M5);
                }
                else {
                    int x_pos = area_graph.x + 10;
                    int y_pos = area_graph.y + area_graph.h / 2 - font_h * 2;

                    SmplBuf_WCharS lbl;
                    lbl << format("%04d/%02d/%02d", t.year, t.month, t.day)
                        << L"にはセンサーデータが記録されていません";

                    drawChar(font
                        , x_pos, y_pos
                        , lbl.c_str(), col_darkR, WHITE, 0, M5);
                }
            }
        }

        /**
         * Plot live graph.
         * 
         *   pick latest data from temporary FIFO buffer node.live_history[],
         *   storing newer data from DB and incoming sensor data.
         *   
         * 
         * \param t_now         The current local time.
         * \param b_preview     true: show as preview, false: normal operation.
         */
        void plot_live(TWESYS::TweLocalTime& t_now, bool b_preview = false) {
            if (!base._db) return;

            //auto& db = *base._db;
            auto& node = base._node;

            base._view.pseudo_scale = 1;
            base._view.pseudo_start_idx = 0;

            int32_t last_cursor = pseudo_cursor_idx;
            int32_t last_sid = 0;
            int64_t last_ts = 0;
            if (last_cursor >= 0 && last_cursor < DRAW_WIDTH && v_dat[last_cursor].sid) {
                last_sid = *v_dat[last_cursor].sid;
                last_ts = *v_dat[last_cursor].ts;
            }
            base._view.set_start_epoch_and_duration(t_now.epoch - LIVE_VIEW_DUR, LIVE_VIEW_DUR);

            // adding entry from newer one (first one is used to determine data_type).
            for (int i = node.live_history.size() - 1; i >= 0; i--) {
                auto& d = node.live_history[i];
                if (*d.sid == node.sid) {
                    int i_stored = base._view.add_entry(node.live_history[i]);
                    if (last_sid == *d.sid && last_ts == *d.ts) {
                        pseudo_cursor_idx = i_stored;
                    }
                }
            }

            base._view.finalize_entry();
            base._view.plot_graph(b_preview);
        }

        /**
         * view magnified.
         *
         * \param x_pos     relative position of x axis from the graph view.
         * \return          true: view is changed.
         */
        bool pseudo_scale_up(int32_t x_pos = 0x80000000) {
            bool b_no_change = false;

            int32_t l1 = pseudo_scale; // new scale  (Value proportional to pseudo screen width).

            switch (pseudo_scale) {
            case 1: l1 = 2; break;
            case 2: l1 = 4; break;
            case 4: l1 = 8; break;
            case 8: l1 = 24; break;
            case 24: l1 = 48; break;
            case 48: l1 = 96; break;
            case 96: l1 = 192; break;
            }

            if (pseudo_scale != l1) {
                _pseudo_view_recalc(l1, x_pos);
                return true;
            }

            return false;
        }

        bool pseudo_scale_down(int32_t x_pos = 0x80000000) {
            bool b_no_change = false;

            int32_t l1 = pseudo_scale; // new scale  (Value proportional to pseudo screen width).

            switch (pseudo_scale) {
            case 2: l1 = 1; break;
            case 4: l1 = 2; break;
            case 8: l1 = 4; break;
            case 24: l1 = 8; break;
            case 48: l1 = 24; break;
            case 96: l1 = 48; break;
            case 192: l1 = 96; break;
            }

            if (pseudo_scale != l1) {
                _pseudo_view_recalc(l1, x_pos);
                return true;
            }

            return false;
        }

        void _pseudo_view_recalc(int32_t new_scale, int32_t x_pos = 0x80000000) {
            int32_t l0 = pseudo_scale; // current scale (Value proportional to pseudo screen width.
            int32_t l1 = new_scale; // current scale (Value proportional to pseudo screen width.

            // for keyboard operation
            if (x_pos == 0x80000000) {
                if (pseudo_cursor_idx != -1) x_pos = pseudo_cursor_idx - pseudo_start_idx;
                else x_pos = DRAW_WIDTH / 2;
            }

            /* calculate start index of the new view.
             *       <--->x_pos
             *   [   |   * |  ]=l0(current)
             *    ------->i0
             *       ^pseudo_start_idx(current)
             *
             *   [          |    * |      ]=l1
             *    --------------->i1: i1=(l1/l0)*i0
             *              ^pseudo_start_idx(new)=i1-x_pos
             */
            int32_t i0 = pseudo_start_idx + x_pos;
            int32_t i1 = 16 * i0 * l1 / l0 / 16;
            int32_t i_start = i1 - x_pos;

            if (i_start < 0) i_start = 0;
            if (i_start > new_scale * DRAW_WIDTH - DRAW_WIDTH) i_start = new_scale * DRAW_WIDTH - DRAW_WIDTH;

            // set other values
            pseudo_scale = new_scale;

            // reload pseudo screen
            preudo_reload();

            // set start index
            pseudo_start_idx = int32_t(i_start);
        }

        /**
         * reload sensor data from v_dat_full[] into v_dat[] for display.
         *
         */
        void preudo_reload() {
            // recreate viewing data v_dat[]
            int32_t i_start = pseudo_start_idx; // this value will be initialized, so save it here.
            set_start_epoch_and_duration(t_start, int(t_end - t_start), pseudo_scale * DRAW_WIDTH);
            for (auto& x : v_dat_full) {
                if (x.sid) add_entry(x);
            }
            finalize_entry();
            pseudo_start_idx = i_start; // restore the starting index.

            // try to restore cursor index.
            if (full_cursor_idx != -1) {
                pseudo_cursor_idx = full_cursor_idx / get_samples_per_pixel();
            }
        }

        /**
         * Check if the coordinate(x,y) is in an area(area).
         * - for checking mouse pointe is in an area.
         *
         * \param area	the area
         * \param x		coord x
         * \param y		coord y
         * \return		true: in the area, false: out of the area
         */
        static bool is_in_area(Rect& area, int32_t x, int32_t y) {
            if (x >= area.x && x < area.x + area.w
                && y >= area.y && y < area.y + area.h) return true;
            else return false;
        }


        /**
         * get samples per pixel in 24hour view.
         * 
         * \return samples in a pixel.
         */
        int32_t get_samples_per_pixel()  { return (_VIEW::DATA_WIDTH / _VIEW::DRAW_WIDTH) / pseudo_scale; }

        /**
         * Adjust the display range so that the pre-set full_cursor_index is displayed.
         * 
         */
        void _pick_sample_adjust_view() {
            // check the item is in the view.
            int samples_per_pixel = get_samples_per_pixel();
            int ip = full_cursor_idx / samples_per_pixel;

            const int MARGIN = DRAW_WIDTH / 6;

            // if desired pixel is too left
            if (ip < pseudo_start_idx + DRAW_WIDTH / 6) {
                pseudo_start_idx = ip - MARGIN;
                if (pseudo_start_idx < 0) pseudo_start_idx = 0;
            }

            // if too right.
            if (ip >= pseudo_start_idx + DRAW_WIDTH - MARGIN) {
                pseudo_start_idx = ip + MARGIN - DRAW_WIDTH;
                if (pseudo_start_idx > pseudo_width - DRAW_WIDTH) pseudo_start_idx = pseudo_width - DRAW_WIDTH;

            }

            // set cursor idx
            pseudo_cursor_idx = ip;
        }

        /**
         * Displays the selected sensor data on the terminal.
         * - sns_pick.set() shall be called before this call.
         * 
         * \param scr
         */
        void _pick_sample_show_info(ITerm& scr) {
            const int l_start = 18;

            auto& d = sns_pick.d;
            auto& td = sns_pick.lt;

            // clear line
            for (int i = l_start; i < l_start + 12; i++) {
                scr.clear_line(i);
            }
            
            int l = l_start;
            scr(1, l++) << "[    センサー情報    ]";
            scr(1, l++) << format("  TS:%02d:%02d:%02d.%03d", td.hour, td.minute, td.second, *d.ts_msec);

            scr(1, l++);
            if (d.pkt_seq) scr << format(" SEQ: %5d", *d.pkt_seq); else scr << format(" SEQ: -----", *d.pkt_seq);
            if (d.lqi) scr << format(" LQI: %3d", *d.lqi); else scr << " LQI: ---";

            scr(1, l++);
            if (d.lid) scr << format(" LID: %3d", *d.lid); else scr << " LID: ---";
            if (d.pkt_type) scr << format(" TYP: %d", *d.pkt_type); else scr << " TYP: -";

            scr(1, l++);
            if (d.val_vcc_mv) scr << format(" VCC: %4d", *d.val_vcc_mv); else scr << " VCC: ----";

            scr(1, l++) << " (" << _print_double(d.value) << ", " << _print_double(d.value1);
            scr(1, l++) << "  " << _print_double(d.value2) << ", " << _print_double(d.value3) << ")";

            scr(1, l++);
            if (d.val_dio) scr << format(" DIO: %X", *d.val_dio); else scr << " DIO: ----";

            scr(1, l++);
            if (d.ev_id) scr << format(" EV: %03d", *d.ev_id); else scr << " EV: ---";
        }

        /**
         * .
         * 
         * \param b_live
         * \return 
         */
        int32_t pick_sample_prev(bool b_live) {
            if (b_live) {
                int i_start = (pseudo_cursor_idx == -1) ? DRAW_WIDTH - 1 : pseudo_cursor_idx - 1;
                for (int i = i_start; i >= 0; i--) {
                    if (v_dat[i].sid && *v_dat[i].sid == int32_t(base._node.sid)) {
                        full_cursor_idx = -1;
                        pseudo_cursor_idx = i;

                        sns_pick.set(v_dat[i]);
                        _pick_sample_show_info(base.the_screen);
                        plot_graph();
                        return i;
                    }
                }
            }
            else {
                // start index (previous to the current cursor, if not set, start from right edge of current display)
                int i_start = (full_cursor_idx == -1) ? (pseudo_start_idx + DRAW_WIDTH) * get_samples_per_pixel() : full_cursor_idx - 1;

                for (int i = i_start; i >= 0; i--) {
                    if (v_dat_full[i].sid && *v_dat_full[i].sid != 0) {
                        full_cursor_idx = i;

                        _pick_sample_adjust_view();
                        sns_pick.set(v_dat_full[i]);
                        _pick_sample_show_info(base.the_screen);
                        plot_graph();

                        return i;
                    }
                }
            }

            return -1;
        }

        int32_t pick_sample_next(bool b_live) {
            if (b_live) {
                int i_start = (pseudo_cursor_idx == -1) ? 0 : pseudo_cursor_idx + 1;
                for (int i = i_start; i < DRAW_WIDTH; i++) {
                    if (v_dat[i].sid && *v_dat[i].sid == int32_t(base._node.sid)) {
                        full_cursor_idx = -1;
                        pseudo_cursor_idx = i;

                        sns_pick.set(v_dat[i]);
                        _pick_sample_show_info(base.the_screen);
                        plot_graph();

                        return i;
                    }
                }
            } else {
                // start index (next to the current cursor, if not set, start from left edge of current display)
                int i_start = (full_cursor_idx == -1) ? pseudo_start_idx * get_samples_per_pixel() : full_cursor_idx + 1;
                for (int i = i_start; i < DATA_WIDTH; i++) {
                    if (v_dat_full[i].sid && *v_dat_full[i].sid != 0) {
                        full_cursor_idx = i;

                        _pick_sample_adjust_view();

                        sns_pick.set(v_dat_full[i]);
                        _pick_sample_show_info(base.the_screen);

                        plot_graph();
                        return i;
                    }
                }
            }

            return -1;
        }

        /**
         * Find nearest sensor data of x-coord in graph view.
         * 
         * \param x_mouse   x coord.
         * \param b_live    true: in live view, false: 24Hours view (subscr_the_day)
         */
        int32_t pick_sample_and_display_info(int32_t x_mouse, bool b_live) {
            auto& view = *this;
            auto& node = base._node;
            auto& scr = base.the_screen;
            int x_found = -1;

            if (!node.sid) return -1;

            if (b_live) {
                int xp = x_mouse - view.area_graph.x;
                int n_search_samples = 5;

                if (xp >= 0 && xp < _VIEW::DRAW_WIDTH && view.v_dat[xp].sid && *view.v_dat[xp].sid == int32_t(node.sid)) {
                    x_found = xp;
                }
                else {
                    for (int i = 1; i < n_search_samples; i++) {
                        int x0 = xp - i;

                        if (x0 >= 0 && x0 < _VIEW::DRAW_WIDTH && view.v_dat[x0].sid && *view.v_dat[x0].sid == int32_t(node.sid)) {
                            x_found = x0;
                            break;
                        }

                        int x1 = xp + i;
                        if (x1 >= 0 && x1 < _VIEW::DRAW_WIDTH && view.v_dat[x1].sid && *view.v_dat[x1].sid == int32_t(node.sid)) {
                            x_found = x1;
                            break;
                        }
                    }
                }

                if (x_found != -1) {
                    pseudo_cursor_idx = x_found;
                    full_cursor_idx = -1; // in live view, full_cursor_idx is insignificance.
                }
            }
            else if (view.v_dat_full.length() == _VIEW::DATA_WIDTH) {
                int xp = view.pseudo_start_idx + (x_mouse - view.area_graph.x);              // px is the index in pseudo view.
                xp = xp * (_VIEW::DATA_WIDTH / _VIEW::DRAW_WIDTH) / view.pseudo_scale; // now px is the index in full data.

                int samples_per_pixel = (_VIEW::DATA_WIDTH / _VIEW::DRAW_WIDTH) / view.pseudo_scale;
                int n_search_samples = 5 * samples_per_pixel; // 5pixes

                if (xp >= 0 && xp < _VIEW::DATA_WIDTH && view.v_dat_full[xp].sid) {
                    x_found = xp;
                }
                else {
                    for (int i = 1; i < n_search_samples; i++) {
                        int x0 = xp - i;

                        if (x0 >= 0 && x0 < _VIEW::DATA_WIDTH && view.v_dat_full[x0].sid) {
                            x_found = x0;
                            break;
                        }

                        int x1 = xp + i;
                        if (x1 >= 0 && x1 < _VIEW::DATA_WIDTH && view.v_dat_full[x1].sid) {
                            x_found = x1;
                            break;
                        }
                    }
                }

                if (x_found != -1) {
                    pseudo_cursor_idx = x_found / samples_per_pixel;
                    full_cursor_idx = x_found;
                }
            }

            if (x_found != -1) {
                if (b_live) sns_pick.set(view.v_dat[x_found]); else sns_pick.set(view.v_dat_full[x_found]);
                _pick_sample_show_info(scr);
                return x_found;
            }

            return -1;
        }

        void disp_sid_text(uint32_t sid, int l) {
            auto& scr = base.the_screen;
            
            SmplBuf_WChar text_sid(1024);
            base._db->query_sid_string(sid, text_sid);

            scr.clear_line(l);
            scr(0, l) << "\033[40m\033[K";
            if (!text_sid.empty()) {
                int len = strlen_vis(text_sid.c_str());
                if (len <= scr.get_cols()) {
                    scr((scr.get_cols() - len) / 2, l) << text_sid;
                }
                else {
                    while(strlen_vis(text_sid.c_str()) > scr.get_cols()) text_sid.pop_back(); // trunk it.
                    scr(0, l) << text_sid;
                }
            }
            scr << "\033[0m";
        }

    public:
        _VIEW(SCR_WSNS_DB& base) 
            : base(base) 
            , area_draw() , area_graph(), area_graph_sub(), area_scroll_bar()
            , x_ptr(0), y_ptr(0), b_cursor_show(true)
            , t_start(0), t_step(0), t_step_full(0), t_end(0)
            , pseudo_width(DRAW_WIDTH), pseudo_start_idx(0), pseudo_cursor_idx(-1), pseudo_scale(1), full_cursor_idx(-1)
            , ct_plot_points(0)
            , data_type(uint8_t(E_PAL_DATA_TYPE::NODEF))
            , v_dat(DATA_WIDTH + 1)
            , v_ave(DATA_WIDTH + 1)
            , v_dat_full(DATA_WIDTH + 1)
            , bm_value(0)
            , drag{}
            , scrbar()
            , sns_pick()
        {}

    public:
        SCR_WSNS_DB& base;                      // access to parent SCR_WSNS_DB object.

        Rect area_draw;                         // whole area of graph screen
        Rect area_graph;                        // the main graph area
        Rect area_graph_sub;                    // the sub graph area
        Rect area_scroll_bar;                   // scroll bar area

        int32_t x_ptr, y_ptr;                   // mouse pointer coord.
        bool b_cursor_show;                     // flag to show/hide mouse pointer.
        
        uint64_t t_start, t_end;                // time range of the view. (note: t_end is the next to the last data)
        int32_t t_step, t_step_full;            // step of timestamp of between the pixel.
        int32_t ct_plot_points;                 // count of plotted data.
        int32_t pseudo_width;                   // pseudo screen width (e.g. 2x magnified, it will be DRAW_WIDTHx2)
        int32_t pseudo_start_idx;               // start index of the view in pseudo screen if magnified.
        int32_t pseudo_cursor_idx;              // cursor index of pseudo view (v_dat[])
        int32_t full_cursor_idx;                // cursor index of full data (v_dat_full[])
        uint8_t pseudo_scale;                   // magnified scale (pseudo screen is used when 2 or more)

        uint16_t data_type;                      // sensor data type (PAL model, ARIA, etc)

        SimpleBuffer<WSnsDb::SENSOR_DATA> v_dat;        // sensor data vector of the screen/pseudo screen.
        SimpleBuffer<WSnsDb::SENSOR_DATA> v_dat_full;   // sensor data of full width.
        SimpleBuffer<int32_t> v_ave;                    // average count for v_dat[].
        uint32_t bm_value;                              // bitmap to tell the existing data.

        TWESYS::TweLocalTime lt_preview;        // previewed date.
        uint32_t sid_preview;

        struct _drag_ctl {
            bool b_dragging;     // when dragging by mouse left button.
            bool b_drag_scrollbar;     // when dragging the scroll bar
            int32_t x_drag, y_drag, x_last, y_last, view_idx_when_dragged; // for drag control.
        } drag;

        TWE_GUI_ScrollBarH scrbar;

        /**
         * if a sample is selected on the graph screen, store some data here.
         */
        struct _pick_data {
            WSnsDb::SENSOR_DATA d;      // sensor data
            TWESYS::TweLocalTime lt;    // local time data.

            /**
             * set a new picked sensor data.
             * 
             * \param d_    sensor data.
             */
            void set(WSnsDb::SENSOR_DATA& d_) {
                d = d_;
                lt.set_epoch(*d.ts);
            }
            _pick_data() : d(), lt() {}
        } sns_pick;
    } _view;

    // Drawing area
    Rect _area_draw;		// the area of graph screen
    Rect _area_scr_main;	// saves the area of main screen(the_screen), the main screen is moved to right side whie this screen is present.

#ifdef WSNS_DEBUG_FUNCTIONS
    struct _RAND_MGR {
        std::random_device seed;
        std::unique_ptr<std::uniform_real_distribution<>> d;
        std::unique_ptr<std::mt19937> e;

        void setup() {
            e.reset(new std::mt19937(seed()));
            d.reset(new std::uniform_real_distribution<>(0.0, 1.0));
        }

        double get() {
            return d->operator()(*e);
        }
    } _rnd;
#endif


    /**
     * set title on the_screen.
     * 
     * \param lbl
     */
    void set_title(const wchar_t* lbl) {
        the_screen(0, 1) << "\033[40m\033[K \033[1;33m" << lbl << "\033[0m";
    }

    /**
     * check if its a new second.
     * 
     * \return  new second tick if true.
     */
    bool _is_new_sec() {
        uint32_t t_now = millis();
        uint32_t sec_now = t_now / 1000;

        if (_sec != sec_now) {
            _sec = sec_now;
            return true;
        }

        return false;
    }

    /**
     * process UART data and put sensor data into DB.
     * 
     * \param u8b       a byte from UART.
     */
	void parse_a_byte(char_t u8b) {
		// parse.
		parse_ascii << u8b;

		// if complete parsing
		if (parse_ascii) {
			// output as parser format
			the_screen_b.clear_screen();
			the_screen_b << "PKT(" << ++_pkt_rcv_ct << ')';

			// 1. identify the packet type
			auto&& pkt = newTwePacket(parse_ascii.get_payload());
            auto pkt_type = identify_packet_type(pkt);
			the_screen_b << ":Typ=" << int(pkt_type);

            WSnsDb::SENSOR_DATA d;
            
            if (pkt_type == E_PKT::PKT_TWELITE) {
                auto&& atw = refTwePacketTwelite(pkt);

                the_screen_b // WrtCon // 
                    << "App_Twelite"
                    << ":DI1..4=" << format("%x:%x", atw.DI_mask, atw.DI_active_mask);
        
                uint8_t bm_di = atw.DI_mask & 0xF;
                bool b_regular_tx = !!(atw.DI_mask & 0x80);
                
                d.sid = DB_INTEGER::value_type(atw.u32addr_src);
                d.lid = DB_INTEGER::value_type(atw.u8addr_src);
                d.lqi = DB_INTEGER::value_type(atw.u8lqi);
                d.val_vcc_mv = DB_INTEGER::value_type(atw.common.volt);

                d.val_dio = DB_INTEGER::value_type(int32_t(bm_di) | (b_regular_tx ? 0x80000000l : 0));
                d.val_aux = DB_INTEGER::value_type(atw.DI_active_mask);

                // the primary value is IO bit mask
                d.value = DB_REAL::value_type(bm_di);

                if (atw.Adc_active_mask & (1 << 0)) {
                    d.val_adc1_mv = DB_INTEGER::value_type(atw.u16Adc1 );
                    d.value1 = DB_REAL::value_type(atw.u16Adc1 / 1000.);
                }
                if (atw.Adc_active_mask & (1 << 1)) {
                    d.value2 = DB_REAL::value_type(atw.u16Adc2 / 1000.);
                }
                if (atw.Adc_active_mask & (1 << 2)) {
                    d.value3 = DB_REAL::value_type(atw.u16Adc3 / 1000.);
                }
                if (atw.Adc_active_mask & (1 << 3)) {
                    d.val_adc2_mv = DB_INTEGER::value_type(atw.u16Adc4);
                    d.value3 = DB_REAL::value_type(atw.u16Adc4 / 1000.);
                }

                d.pkt_seq = DB_INTEGER::value_type(atw.u16timestamp);
                d.pkt_type = DB_INTEGER::value_type(PKT_TYPE_APPTWELITE);
            }
            else if (pkt_type == E_PKT::PKT_APPIO) {
                auto&& atw = refTwePacketTwelite(pkt);

                the_screen_b
                    << "App_IO"
                    << ":DI1..X=" << format("%2x:%2x", atw.DI_mask, atw.DI_active_mask);

                d.sid = DB_INTEGER::value_type(atw.u32addr_src);
                d.lid = DB_INTEGER::value_type(atw.u8addr_src);
                d.lqi = DB_INTEGER::value_type(atw.u8lqi);

                d.val_dio = DB_INTEGER::value_type(atw.DI_mask);
                d.value = DB_REAL::value_type(atw.DI_mask);
                d.val_aux = DB_INTEGER::value_type(atw.DI_active_mask);

                d.pkt_seq = DB_INTEGER::value_type(atw.u16timestamp);
                d.pkt_type = DB_INTEGER::value_type(PKT_TYPE_APPIO);
            }
			else if (pkt_type == E_PKT::PKT_PAL) {
				auto&& pal = refTwePacketPal(pkt);
               
				// put information
				the_screen_b
					<< printfmt(":Lq=%d:Ad=%08X", pal.u8lqi, pal.u32addr_src)
					<< ":PAL=" << format("%d", pal.u8palpcb)
					<< ":ID=" << format("%d", pal.u8addr_src)
					;

                auto pal_type = pal.get_PalDataType();

                if (pal_type == E_PAL_DATA_TYPE::EX_ARIA_STD) {
                    the_screen_b << ":ARIA"
                                    ":EX(" << int(pal.u8data_type) << "," << int(pal.u8_data_cause) << "," << int(pal.u8_data_cause) << ")";
                    
                    TweARIA dsns = pal.get_TweARIA();

                    d.sid = DB_INTEGER::value_type(pal.u32addr_src);
                    d.value = DB_REAL::value_type(dsns.get_temp_i16_100xC() / 100.0);
                    d.value1 = DB_REAL::value_type(dsns.get_humidity_u16_100xPC() / 100.0);
                    d.val_vcc_mv = DB_INTEGER::value_type(dsns.u16Volt);
                    if (dsns.has_mag()) d.val_dio = DB_INTEGER::value_type(((int32_t)dsns.get_mag_stat_u8() + 0x10) << 24); // MAG data will be stored on DIO
                    if (dsns.has_adc1()) d.val_adc1_mv = DB_INTEGER::value_type(dsns.get_adc1_i16mV());
                }
                else if (pal_type == E_PAL_DATA_TYPE::AMB_STD) {
                    the_screen_b << ":AMB_PAL"
                                    ":EX(" << int(pal.u8data_type) << "," << int(pal.u8_data_cause) << "," << int(pal.u8_data_cause) << ")";

                    PalAmb dsns = pal.get_PalAmb();

                    d.sid = DB_INTEGER::value_type(pal.u32addr_src);

                    d.value = DB_REAL::value_type(dsns.get_temp_i16_100xC() / 100.0);
                    d.value1 = DB_REAL::value_type(dsns.get_humidity_i16_100xPC() / 100.0);
                    d.val_vcc_mv = DB_INTEGER::value_type(dsns.u16Volt);
                    d.value2 = DB_REAL::value_type(dsns.get_luminance_LUX());
                }
                else if (pal_type == E_PAL_DATA_TYPE::EX_CUE_STD) {
                    the_screen_b 
                        << ":CUE"
                        << ":EX(" << int(pal.u8data_type) << "," << int(pal.u8_data_cause) << "," << int(pal.u8_data_cause) << ")";

                    TweCUE dsns = pal.get_TweCUE();

                    d.sid = DB_INTEGER::value_type(pal.u32addr_src);

                    d.val_vcc_mv = DB_INTEGER::value_type(dsns.get_vcc_i16mV());
                    if (dsns.has_adc1()) d.val_adc1_mv = DB_INTEGER::value_type(dsns.get_adc1_i16mV());
                    if (dsns.has_mag()) d.val_dio = DB_INTEGER::value_type(((int32_t)dsns.get_mag_stat_u8() + 0x10) << 24); // MAG data will be stored on DIO

                    if (dsns.has_accel()) {
                        int ave_x = 0, ave_y = 0, ave_z = 0;
                        for (int i = 0; i < dsns.get_accel_count_u8(); i++) {
                            ave_x += dsns.get_accel_X_i16mG(i);
                            ave_y += dsns.get_accel_Y_i16mG(i);
                            ave_z += dsns.get_accel_Z_i16mG(i);
                        }
                        double f = 1000. * dsns.get_accel_count_u8();
                        d.value = DB_REAL::value_type(ave_x / f);
                        d.value1 = DB_REAL::value_type(ave_y / f);
                        d.value2 = DB_REAL::value_type(ave_z / f);

                        ave_x >>= 3;
                        ave_y >>= 3;
                        ave_z >>= 3;

                        the_screen_b << ":" << "(" << ave_x;
                        the_screen_b << "," << ave_y;
                        the_screen_b << "," << ave_z;
                        the_screen_b << ")";
                    }
                }
                else if (pal_type == E_PAL_DATA_TYPE::MOT_STD) {
                    PalMot dsns = pal.get_PalMot();

                    d.sid = DB_INTEGER::value_type(pal.u32addr_src);
                    d.val_vcc_mv = DB_INTEGER::value_type(dsns.u16Volt);

                    the_screen_b << "PAL_MOT";
                    the_screen_b << ":SAMPLES=" << int(dsns.u8samples);
                    the_screen_b << ":SR=" << int(dsns.u8sample_rate_code);

                    int ave_x = 0, ave_y = 0, ave_z = 0;
                    for (int i = 0; i < dsns.u8samples; i++) {
                        ave_x += dsns.i16X[i];
                        ave_y += dsns.i16Y[i];
                        ave_z += dsns.i16Z[i];
                    }

                    double f = 1000. * dsns.u8samples;
                    d.value = DB_REAL::value_type(ave_x / f);
                    d.value1 = DB_REAL::value_type(ave_y / f);
                    d.value2 = DB_REAL::value_type(ave_z / f);

                    the_screen_b << ":" << "(" << ave_x;
                    the_screen_b << "," << ave_y;
                    the_screen_b << "," << ave_z;
                    the_screen_b << ")";
                }
                else if (pal_type == E_PAL_DATA_TYPE::MAG_STD) {
                    PalMag dsns = pal.get_PalMag();

                    d.sid = DB_INTEGER::value_type(pal.u32addr_src);
                    d.val_vcc_mv = DB_INTEGER::value_type(dsns.u16Volt);

                    the_screen_b << ":MAG:STAT=" << int(dsns.u8MagStat);

                    d.val_dio = DB_INTEGER::value_type(((int32_t)dsns.u8MagStat + 0x10) << 24); // MAG data will be stored on DIO

                    d.value = DB_REAL::value_type(dsns.u8MagStat & 0x3);
                }

                // store common information
                if (d.sid) {
                    d.pkt_seq = DB_INTEGER::value_type(pal.u16seq);
                    d.lid = DB_INTEGER::value_type(pal.u8addr_src);
                    d.lqi = DB_INTEGER::value_type(pal.u8lqi);
                    d.pkt_type = DB_INTEGER::value_type(uint8_t(pal_type));

                    // save events
                    if (pal.has_PalEvent()) {
                        PalEvent& ev = pal.get_PalEvent();
                        d.ev_src = DB_INTEGER::value_type(ev.u8event_source);
                        d.ev_id = DB_INTEGER::value_type(ev.u8event_id);
                        d.ev_param = DB_INTEGER::value_type(ev.u32event_param);
                        if (ev) the_screen_b << ":EVENT=" << int(ev.u8event_id);
                    }
                }
			}

            // if set SID, data is prepared.
            if (d.sid) {
                if (_db->sensor_data_add(d) == EXIT_SUCCESS) { // note: ts will be update during sensor_data_add()
                    _node.live_history.push_force(d); // push any of data.

                    if (_node.sid == d.sid) { // if selected SID, request view updated.
                        _node.b_live_update = true; // set flag
                        _node.ts_newest = DB_TIMESTAMP(d.ts); // update the newest entry

                        if (*d.ts >= DB_TIMESTAMP::value_type(_view.t_start) && *d.ts < DB_TIMESTAMP::value_type(_view.t_end)) {
                            _view.add_entry_full(d);
                        }
                    }
                }
            }
		}
	}

#ifdef WSNS_DEBUG_FUNCTIONS
    /**
     * get random value from 0.0 to 1.0.
     * 
     * \return  random value ranged [0.0, 1.0].
     */
    double get_random_value() {
        return _rnd.get();
    }

    /**
     * Inserts dummy data for debugging.
     * 
     * \param n     Number of data to be added
     */
    bool db_insert_dummy_entries(uint32_t n, int64_t sec_to_go_back) {
        const uint32_t u32ids[] = { 0x80123450, 0x80123451, 0x80123452, 0x80123453, 0x80123454, 0x80123455, 0x80123456, 0x80123457 };

        if (!_db) return false;

        bool b_stat = true;

        // prepare node information (not mandate)
        _db->sensor_node_add(u32ids[0], "DMY_NODE1");
        _db->sensor_node_add(u32ids[1], "DMY_NODE2");
        _db->sensor_node_add(u32ids[2], "DMY_NODE3");
        _db->sensor_node_add(u32ids[3], "DMY_NODE4");
        _db->sensor_node_add(u32ids[4], "DMY_NODE5");
        _db->sensor_node_add(u32ids[5], "DMY_NODE6");
        _db->sensor_node_add(u32ids[6], "DMY_NODE7");
        _db->sensor_node_add(u32ids[7], "DMY_NODE8");

        // The timestamp of the data to be added is the past sec_to_go_back seconds starting from the current time.
        auto ts_now = TWESYS::TweLocalTime::epoch_now();

        // commit and starting new transaction.
        if (_db && _db_transaction) {
            _db_transaction.commit();
        }

        // add entries.
        if (auto&& trs = _db->get_transaction_obj()) {
            uint32_t t0 = millis();

            for (uint32_t i = 0; i < n; i++) {
                // select node id from u32ids[] ramdomly.
                unsigned node = unsigned(get_random_value() * elements_of_array(u32ids));

                WSnsDb::SENSOR_DATA d;

                d.sid = DB_INTEGER::value_type(u32ids[node]);

                d.ts = DB_TIMESTAMP::value_type(ts_now - get_random_value() * sec_to_go_back);

                d.ts_msec = DB_INTEGER::value_type(get_random_value() * 999);

                d.lid = DB_INTEGER::value_type(u32ids[node] & 0xFF);

                d.lqi = DB_INTEGER::value_type(get_random_value() * 255);

                d.pkt_type = DB_INTEGER::value_type(uint8_t(E_PAL_DATA_TYPE::AMB_STD));

                d.pkt_seq = DB_INTEGER::value_type(0);

                d.value = 10.0 + get_random_value() * 20.0;
                d.value1 = 50.0 + (get_random_value() - 0.5) * 40.0;
                d.value2 = get_random_value() * 10000;

                d.val_vcc_mv = DB_INTEGER::value_type(2000 + get_random_value() * 1600);

                if (_db->sensor_data_add(d) != EXIT_SUCCESS) {
                    b_stat = false;
                    break;
                }
            }

            // if all success, commit the change into db.
            if (b_stat) {
                trs.commit();
            }

            WrtCon << crlf << format("db_insert_dummy_entries: %d takes %dms", n, millis() - t0);
        }

        // kick new transaction
        _db_transaction = _db->get_transaction_obj();
       
        // returns the state
        return b_stat;
    }
#endif

    /**
     * open and prepare DB ready to use.
     * 
     * \return 
     */
    bool db_open() {
        bool b_stat = true;

        // prepare DB filename (log/{STAGE EXE BASENAME}_Wsns.sqlite)
        SmplBuf_ByteSL<1024> file_fullpath;
        file_fullpath << make_full_path(the_cwd.get_dir_log(), the_cwd.get_filename_exe());
        file_fullpath << WSNS_DB_FILENAME;

        // create object
        _db.reset(new WSnsDb(WrtCon));

        if (_db && _db->open(file_fullpath.c_str()) != EXIT_SUCCESS) {
            // on error
            _db.reset(nullptr);
            b_stat = false;
        }

        //_db->drop_tables(); // for DEBUG

        if (_db && _db->prepare_tables() != EXIT_SUCCESS) {
            // on error
            _db.reset(nullptr);
            b_stat = false;
        }
        
        return b_stat;
    }

    /**
     * commit DB and re-new transaction.
     */
    void db_commit() {
        if (_db) {
            if (_db_transaction) {
                // commit DB request every sec.
                _db_transaction.commit();
            }

            // a new transaction
            _db_transaction = _db->get_transaction_obj();
        }
    }

    /**
     * close the database and clean up instances.
     */
    void db_close() {
        // closing database
        if (_db) {
            if (_db_transaction) {
                _db_transaction.commit();
            }
            _db.reset(nullptr);
        }
    }

    /**
     * base class of screen.
     * - common control: 
     */
    struct B_subs_view_gen {
        TWE_ListView lv;

        uint32_t tick_request;
        int idx_request;

        /**
         * attach list view to the screen.
         * note: the last line of the screen is kept unused for other purposes.
         * 
         * \param scr
         * \param row_start_of_lv       staring rows of the screen.
         */
        void lv_attach_scr(ITerm& scr, int row_start_of_lv) {
            lv.attach_term(scr, row_start_of_lv, scr.get_rows() - row_start_of_lv - 1, true);
            lv.set_view();
            lv.update_view(true);
        }

        /**
         * redraw list view.
         * 
         */
        void lv_redraw() {
            lv.update_view(true);
        }

        /**
         * event handlers if selected, right button pressed or mouse over.
         * 
         * \param c             event (key input or mouse)
         * \param func          handler when selected
         * \param func_info     handler when right button is pressed
         * \param func_over     handler when mouse over
         * \return              true: event 'c' is used, false: 'c' is not used.
         */
        bool lv_handle_events(int c
            , std::function<void(int idx_sel, TWE_ListView::pair_type&& item_sel)> func
            , std::function<void(int n_sel, int idx_sel, TWE_ListView::pair_type&& item_sel)> func_info = [](auto,auto,auto&&) {}
            , std::function<void(int idx_sel, TWE_ListView::pair_type&& item_sel)> func_over = [](auto,auto&&) {}

        ) {
            bool b_handled = false;

            // handle listView events
            if (lv.key_event(c)) {
                int isel = lv.get_selected_index();

                if (isel >= 0 && isel < lv.size()) {
                    if (lv.is_selection_completed()) {
                        func(isel, lv.get_selected());
                        lv.clear_completion();
                    }
                    else if (auto n_sel = lv.is_info_selected()) { // 1:primary 2:secondary
                        func_info(n_sel, lv.get_selected_index(), lv.get_selected());
                    }
                    else {
                        func_over(isel, lv.get_selected());
                    }
                }
                
                b_handled = true;
            }

            return b_handled;
        }

        /**
         * set timeout used when mouse over.
         * 
         * \param idx       selected index when mouse over.
         */
        void over_item(int idx) {
            if (idx >= 0 && idx != idx_request) {
                idx_request = idx;
                tick_request = millis();
            }
        }

        /**
         * check timeout of over_item().
         * 
         * \return      true: time out
         */
        bool is_timeout_over_on_item() {
            if (tick_request != 0 && (millis() - tick_request > 300)) {
                tick_request = 0;
                int i = idx_request;
                // idx_request = -1;
                return true;
            }
            else {
                return false;
            }
        }

        B_subs_view_gen() : lv(100), tick_request(0), idx_request(-1) {}
    };

    /**
     * template function of subscreen handler (APP_HNDLR).
     */
    EMBED_APP_HNDLR_TEMPLATE_PROCEDURE(hndr_subs);

    /**
     * specific data for subscreen list possible nodes.
     */
    struct subscr_list_nodes : public B_subs_view_gen, public APP_HANDLR_DC {
    public:
        static const int CLS_ID = _SCRN_MGR::SUBS_LIST_NODES; // APP_HANDLER
        
        SCR_WSNS_DB& base;
        TWE_WidSet_Buttons btns;

        struct _node_ent {
            uint32_t sid;
            uint64_t ts;
        };
        SimpleBuffer<_node_ent> v_node;
        int sort_type;

        TWESYS::TweLocalTime tl_query;

        uint32_t tick_last;

        static const int LIVE_VIEW_DUR = _VIEW::LIVE_VIEW_DUR;

        /**
         * Sorting SID list..
         * 
         */
        void _sort_list_by_sid(int id_btn = -1) {
            const wchar_t* plbl = nullptr;
            switch (sort_type) {
            case 0:
                SmplBuf_Sort(v_node,
                    [](_node_ent& a, _node_ent& b) {
                        return a.sid > b.sid;
                    }
                );
                plbl = L"[SID ↑]";
                break;
            case 1:
                SmplBuf_Sort(v_node,
                    [](_node_ent& a, _node_ent& b) {
                        return a.sid < b.sid;
                    }
                );

                plbl = L"[SID ↓]";
                break;
            case 2:
                SmplBuf_Sort(v_node,
                    [](_node_ent& a, _node_ent& b) {
                        return a.ts < b.ts;
                    }
                );
                plbl = L"[PktT↑]";
                break;
            case 3:
                SmplBuf_Sort(v_node,
                    [](_node_ent& a, _node_ent& b) {
                        return a.ts > b.ts;
                    }
                );
                plbl = L"[PktT↓]";
                break;
            }

            if (id_btn != -1) {
                auto& b = btns[id_btn];
                b.get_label() = plbl;
                b.update_view();
            }

            sort_type++;
            if (sort_type > 3) sort_type = 0;
        }

        /**
         * update label of list views from v_node[].
         * 
         */
        void _update_list_string() {
            int idx_f = lv.get_first_index_of_view();
            int idx_s = lv.get_index_of_view();

            TWESYS::TweLocalTime tl;
            tl.now();

            for (unsigned i = 0; i < v_node.size(); i++) {
                auto sid = v_node[i].sid;
                auto ts = v_node[i].ts;

                SmplBuf_WCharSL<64> lbl;
                SmplBuf_WCharSL<64> opt;
                lbl << format("%08X", sid);
                int64_t t_dif = tl.epoch - ts;
                if (t_dif >= 86400) opt << format("%d", (t_dif + 43200) / 86400) << L"日";
                else if (t_dif >= 3600) opt << format("%d", (t_dif + 1800) / 3600) << L"時間";
                else if (t_dif >= 60) opt << format("%d", (t_dif + 30) / 60) << L"分";
                else if (t_dif >= 20) opt << L"\033[37mNEW!\033[130m";
                else if (t_dif >= 10) opt << L"\033[35mNEW!\033[130m";
                else                  opt << L"\033[31mNEW!\033[130m";
                
                lbl << L"(" << opt << L")";

                if (i < lv.size()) {
                    auto x = lv.get(i);
                    auto& l = x.first; 
                    l.clear();
                    l << lbl.c_str();
                }
                else {
                    lv.push_back(lbl.c_str());
                }
            }

            // restore selection
            if (idx_s != -1 && idx_s < lv.size()) {
                lv.set_view(idx_f, idx_s);
            }
        }

        /**
         * update list view (add entry when a new node found) and redrwa list view.
         * - this should be called every sec.
         * 
         */
        void update_list_lively() {
            auto& node = base._node;

            for (int i = node.live_history.size() - 1; i >= 0; --i) {
                auto& d = node.live_history[i];

                uint64_t ts = uint64_t(*d.ts);
                if (*d.ts < DB_TIMESTAMP::value_type(tl_query.epoch)) break; // older timestamp than the last query.

                uint32_t sid = uint32_t(*d.sid);
                
                bool b_updated = false;
                for (unsigned j = 0; j < v_node.size(); ++j) {
                    if (v_node[j].sid == sid) {
                        if (v_node[j].ts < ts) v_node[j].ts = ts; // update ts
                        b_updated = true;
                        break;
                    }
                }
                // if not found, add new entry.
                if (!b_updated) {
                    v_node.push_back({ sid, ts });
                }
            }

            _update_list_string();

            lv_redraw();
        }

#if defined(SUBS_LIST_NODES_LIVE_PREVIEW) // MAY NOT USE THIS CODE ANYMORE...
        /**
         * update/load live data.
         *
         * \return
         */
        bool db_query_live_data() {
            if (!base._db) return false;

            auto& db = *base._db;
            auto& node = base._node;

            TWESYS::TweLocalTime t_now;
            t_now.now();

            node.live_history.clear();

            uint32_t t0 = millis(), ct = 0;
            db.query_sensor_data(
                node.sid,
                t_now.epoch - LIVE_VIEW_DUR + 1, // 1hour
                t_now.epoch,
                [&](WSnsDb::SENSOR_DATA& d) {
                    node.live_history.push_force(d);
                    //base._view.add_entry(d);
                    ct++;
                }
            );
            uint32_t t1 = millis();
            base.the_screen_b << crlf << format("<%d:Q_latest=%08x ct=%d t=%d>", t1, node.sid, ct, t1 - t0);

            return true;
        }
        
        /**
         * plotting live view.
         * 
         */
        void plot_live() {
            TWESYS::TweLocalTime t;
            t.now();

            base._view.plot_live(t, true);
        }
#else
        /**
         * set list view by database query.
         * 
         * \param b_update_on_lv
         * \return 
         */
        bool update_list(bool b_update_on_lv = true) {
            if (!base._db) return false;
            auto& db = *base._db;

            lv.clear();
            v_node.clear();

            lv.set_info_area(L">>");

            tl_query.now(); // save timestamp of this query.

            db.query_sorted_sensor_list_newer_first(
                [&](uint32_t sid, uint64_t ts) {
                    v_node.push_back({ sid, ts });
                }
            );

            // update string label
            if (b_update_on_lv) _update_list_string();

            return true;
        }
 
#endif

        /**
         * initialize view of node list.
         * 
         */
        void show_list(bool init = false) {
            auto& scr = base.the_screen;
            auto& view = base._view;
            auto& node = base._node;

            scr.clear();

            // select SID on the list
            uint32_t sid_set = init ? node.sid : 0;

            if (init) {
            
                lv_attach_scr(scr, 3);

                btns.clear();
                const wchar_t* lbl_update = L"[更新]";
                int c = scr.get_cols() - TWEUTILS::strlen_vis(lbl_update) - 1;
                btns.add(c, 0 /* scr.get_rows() - 1 */, lbl_update,
                    [&](int, uint32_t) {
                        base._node.set_sid(0);

                        base.db_commit();
                        update_list();
                        lv_redraw();
                    }
                );

                const wchar_t* lbl_sort = L"[ソート]";
                btns.add(c - TWEUTILS::strlen_vis(lbl_sort) - 1, 0 /* scr.get_rows() - 1 */, lbl_sort,
                    [&](int id_btn, uint32_t) {
                        base._node.set_sid(0);

                        base.db_commit();
                        update_list(false);

                        _sort_list_by_sid(id_btn); // sort items
                        lv.set_view(0, -1); // set no selected.
                        _update_list_string();
                        lv_redraw();
                    }
                );

                // title
                base.set_title(L"ノード一覧");
                node.set_sid(0);
            }

            // list
            update_list();

            // select the node.
            if (sid_set != 0) {
                for (unsigned i = 0; i <= v_node.size(); i++) {
                    if (sid_set == v_node[i].sid) {
                        lv.set_view_with_select_item(i);
                        break;
                    }
                }
            }

            lv_redraw();

            btns.update_view();
        }

        void setup() {
            show_list(true);
        }

        void loop() {
            auto& scr = base.the_screen;
            auto& node = base._node;
            auto& view = base._view;

            btns.check_events();

            // plot graph periodically (every sec)
            uint32_t t_now = millis();
            if (t_now - tick_last > 1000) {
#if defined(SUBS_LIST_NODES_LIVE_PREVIEW)
                plot_live(); // show live view
#endif
                update_list_lively();

                tick_last = t_now;
            }

            // keyboard handling.
            do {
                int c = the_keyboard.read();

                // handle listView events (it needs to pass event even c == -1 to handle timeout)
                if (1) {
                    if (lv_handle_events(c,
                        [&](int idx_sel, TWE_ListView::pair_type&& item_sel) { // CLICK MAIN CONTENT
                            uint32_t sid = v_node[idx_sel].sid;
                            node.set_sid(sid);

                            if (sid == view.sid_preview) {
                                node.year = view.lt_preview.year;
                                node.month = view.lt_preview.month;
                                node.day = view.lt_preview.day;
                            }

                            base._scr_sub.screen_change_request(_SCRN_MGR::SUBS_D_DAY);
                        },
                        [&](int, int idx_sel, TWE_ListView::pair_type&& item_sel) { // CLICK RIGHT BUTTON
                            uint32_t sid = v_node[idx_sel].sid;
                            node.set_sid(sid);

                            if (sid == view.sid_preview) {
                                node.year = view.lt_preview.year;
                                node.month = view.lt_preview.month;
                                node.day = view.lt_preview.day;
                            }

                            base._scr_sub.screen_change_request(_SCRN_MGR::SUBS_D_DAY);
                        },
                        [&](int idx_sel, TWE_ListView::pair_type&& item_sel) {  // OVER
                            uint32_t sid = v_node[idx_sel].sid;
                            node.set_sid(sid);

                            base._view.disp_sid_text(sid, 2);

                            over_item(idx_sel);
                        }
                    )) c = -1;
                }

                // unhandled events
                if (c == -1);
                else if (KeyInput::is_mouse_right_up(c) || c == KeyInput::KEY_LEFT) {
                    ;
                }
                else if (c == KeyInput::KEY_RIGHT) {
                    base._scr_sub.screen_change_request(_SCRN_MGR::SUBS_D_DAY);
                }
                else switch (c) {
                case KeyInput::KEY_BUTTON_A: break;
                case KeyInput::KEY_BUTTON_B:
#ifdef WSNS_DEBUG_FUNCTIONS
                    base._scr_sub.screen_change_request(_SCRN_MGR::SUBS_DEBUG);
#endif
                    break;
                case KeyInput::KEY_BUTTON_C: break;
                case KeyInput::KEY_BUTTON_A_LONG: break;
                case KeyInput::KEY_BUTTON_B_LONG: break;
                case KeyInput::KEY_BUTTON_C_LONG: 

                    break;
                default: break;
                }

            } while (the_keyboard.available());

            // delayed over display
            if (is_timeout_over_on_item()) {
                auto& node = base._node;
                auto& view = base._view;

 #if defined(SUBS_LIST_NODES_LIVE_PREVIEW)
                // preview live
                db_query_live_data(); //query the latest info.
                plot_live(); //plot it
#else
                // preview the day
                if (node.has_set_day()) {
                    view.plot_day_preview(node.sid, node.year, node.month, node.day);
                }
                else {
                    TWESYS::TweLocalTime t_now; t_now.now();
                    view.plot_day_preview(node.sid, t_now.year, t_now.month, t_now.day);
                }
#endif
            }
        }

        void on_close() {
            btns.clear();
        }

        subscr_list_nodes(SCR_WSNS_DB& base) 
            : B_subs_view_gen()
            , APP_HANDLR_DC(CLS_ID)
            , base(base)
            , btns(*this, base.the_screen)
            , tick_last(0)
            , v_node(), sort_type(0)
        {}

        virtual ~subscr_list_nodes() {}
    };

    /**
    * specific data for subscreen list possible nodes.
    */
    struct subscr_live_view : public APP_HANDLR_DC {
    public:
        static const int CLS_ID = _SCRN_MGR::SUBS_LIVE_VIEW; // APP_HANDLER

        SCR_WSNS_DB& base;
        TWE_WidSet_Buttons btns;

        TWESYS::TweLocalTime tl_query;

        uint32_t tick_last;

        static const int LIVE_VIEW_DUR = 450;

        /**
         * load live data from DB.
         * 
         * \return 
         */
        bool db_query_live_data() {
            if (!base._db) return false;

            auto& db = *base._db;
            auto& node = base._node;

            TWESYS::TweLocalTime t_now;
            t_now.now();

            base.db_commit();

            node.live_history.clear();

            uint32_t t0 = millis(), ct = 0;
            db.query_sensor_data(
                node.sid,
                t_now.epoch - LIVE_VIEW_DUR + 1,
                t_now.epoch,
                [&](WSnsDb::SENSOR_DATA& d) {
                    node.live_history.push_force(d);
                    ct++;
                }
            );
            uint32_t t1 = millis();
            base.the_screen_b << crlf << format("<%d:Q_latest=%08x ct=%d t=%d>", t1, node.sid, ct, t1 - t0);

            return true;
        }

        /**
         * init/update the screen.
         * 
         * \param init      true: call when setup to create buttons, false: update screen.
         */
        void show_detailed_info(bool init = false) {
            auto& scr = base.the_screen;
            auto& node = base._node;

            scr.clear();
            base.set_title(L"ライブデータ");

            int l = 1;
            l++;
            base._view.disp_sid_text(node.sid, l); l++;
            scr(0, l++) << "  SID: " << format("%08X", node.sid);
            l++;
            scr(0, l++) << " DATE: ";
            l++;

            if (init) {
                btns.clear();
                const wchar_t* lbl_back = L"[<<一覧]";

                btns.add(scr.get_cols() - TWEUTILS::strlen_vis(lbl_back) - 1, 0, lbl_back,
                    [&](int, uint32_t) {
                        base._scr_sub.screen_change_request(_SCRN_MGR::SUBS_LIST_NODES);
                    }
                );

                int l = 7;

                btns.add(7, l, L"[年]",
                    [&](int, uint32_t) { base._scr_sub.screen_change_request(_SCRN_MGR::SUBS_LIST_YEARS); }
                );
                btns.add(7 + 6, l, L"[月]",
                    [&](int, uint32_t) { base._scr_sub.screen_change_request(_SCRN_MGR::SUBS_LIST_MONTHS); }
                );
                btns.add(7 + 12, l, L"[日]",
                    [&](int, uint32_t) { base._scr_sub.screen_change_request(_SCRN_MGR::SUBS_LIST_DAYS); }
                );
                l++;

                l++; // blank line

                const wchar_t* lbl24hr = L"[<<24時間データ]";
                btns.add(scr.get_cols() - TWEUTILS::strlen_vis(lbl24hr) - 1, l++, lbl24hr,
                    [&](int, uint32_t) {
                        node.set_latest_date();
                        base._scr_sub.screen_change_request(_SCRN_MGR::SUBS_D_DAY);
                    }
                );


                /*
                btns.add(7 + 6, l, L"[月]",
                    [&](int, uint32_t) { base._scr_sub.screen_change_request(_SCRN_MGR::SUBS_LIST_MONTHS); }
                );

                btns.add(7 + 12, l, L"[日]",
                    [&](int, uint32_t) { base._scr_sub.screen_change_request(_SCRN_MGR::SUBS_LIST_DAYS); }
                );
                */
            }
            btns.update_view();
        }

        /**
         * update live screen.
         * 
         */
        void plot_live() {
            TWESYS::TweLocalTime t;
            t.now();

            base._view.plot_live(t);

            auto& scr = base.the_screen;
            scr(10, 5) << format("%04d/%02d/%02d", t.year, t.month, t.day);
        }

        void setup() {
            db_query_live_data();
            show_detailed_info(true);
        }

        void loop() {
            auto& scr = base.the_screen;
            auto& node = base._node;
            auto& view = base._view;

            btns.check_events();

            // plot graph periodically (every sec)
            uint32_t t_now = millis();
            if (t_now - tick_last > 1000) {
                plot_live();
                tick_last = t_now;
            }

            // keyboard handling.
            do {
                int c = the_keyboard.read();

                // unhandled events
                if (c == -1);
                else if (KeyInput::is_mouse_move(c)) {
                    TWECUI::KeyInput::_MOUSE_EV ev(c);

                    int x = ev.get_x();
                    int y = ev.get_y();

                    bool b_update = false;

                    // check
                    if (view.is_in_area(view.area_graph, x, y) || view.is_in_area(view.area_graph_sub, x, y))
                    {
                        if (view.pick_sample_and_display_info(x, true) != -1) b_update = true;
                    }

                    view.set_pointer_pos(x, y);

                    if (view.is_in_area(view.area_draw, x, y)) {
                        view.plot_graph(); // if(b_update)
                    }
                    else {
                        view.plot_cursor_control(1); // show
                    }
                }
                else if (KeyInput::is_mouse_right_up(c)) {
                    base._scr_sub.screen_change_request(_SCRN_MGR::SUBS_LIST_NODES);
                }
                else switch (c) {
                case KeyInput::KEY_LEFT: view.pick_sample_prev(true); break;
                case KeyInput::KEY_RIGHT: view.pick_sample_next(true); break;
                case KeyInput::KEY_BUTTON_A: break;
                case KeyInput::KEY_BUTTON_B:
#ifdef WSNS_DEBUG_FUNCTIONS
                    base._scr_sub.screen_change_request(_SCRN_MGR::SUBS_DEBUG);
#endif
                    break;
                case KeyInput::KEY_BUTTON_C: break;
                case KeyInput::KEY_BUTTON_A_LONG: break;
                case KeyInput::KEY_BUTTON_B_LONG: break;
                case KeyInput::KEY_BUTTON_C_LONG:
                    break;
                default: break;
                }

            } while (the_keyboard.available());
        }

        void on_close() {
            btns.clear();
            screen_show_cursor();
        }

        subscr_live_view(SCR_WSNS_DB& base)
            : APP_HANDLR_DC(CLS_ID)
            , base(base)
            , btns(*this, base.the_screen)
            , tick_last(0)
        {}

        virtual ~subscr_live_view() {}
    };


    /**
     * query years, which node SID has data.
     */
    struct subscr_list_years : public B_subs_view_gen, public APP_HANDLR_DC {
    public:
        static const int CLS_ID = _SCRN_MGR::SUBS_LIST_YEARS; // APP_HANDLER

        SCR_WSNS_DB& base;
        TWE_WidSet_Buttons btns;

        SimpleBuffer<uint32_t> v_years;

        /**
         * update the list view (query years data from DB).
         * 
         * \param db
         * \return 
         */
        bool update_list(WSnsDb& db) {
            lv.clear();

            lv.set_info_area(L">>", L"<<");
            
            db.query_recorded_years_of_sensor_data(base._node.sid,
                [&](int16_t year) {
                    v_years.push_back(uint32_t(year));
                    
                    SmplBuf_WCharSL<64> lbl;
                    lbl << format("%04d", year);

                    lv.push_back(lbl.c_str());
                }
            );

            the_keyboard.push(KeyInput::KEY_DOWN); // select first item

            return true;
        }

        void setup() {
            auto& scr = base.the_screen;

            scr.clear();

            update_list(*base._db);
            lv_attach_scr(scr, 3);

            btns.clear();

            const wchar_t* lbl_back = L"[<<一覧]";
            btns.add(scr.get_cols() - TWEUTILS::strlen_vis(lbl_back) - 1, 0, lbl_back,
                [&](int, uint32_t) { base._scr_sub.screen_change_request(_SCRN_MGR::SUBS_LIST_NODES); });

            base.set_title(L"年選択");

            // show empty graph
            base._view._clear();
            base._view.plot_empty();
        }

        void loop() {
            auto& scr = base.the_screen;

            btns.check_events();

            // keyboard handling.
            do {
                int c = the_keyboard.read();

                // handle listView events (it needs to pass event even c == -1 to handle timeout)
                if (lv_handle_events(c,
                    [&](int idx_sel, TWE_ListView::pair_type&& item_sel) {
                        base._node.year = v_years[idx_sel];
                        base._scr_sub.screen_change_request(_SCRN_MGR::SUBS_LIST_MONTHS);
                    },
                    [&](int n_sel, int idx_sel, TWE_ListView::pair_type&& item_sel) {
                        base._node.year = v_years[idx_sel];
                        if (n_sel & 1) {
                            base._scr_sub.screen_change_request(_SCRN_MGR::SUBS_LIST_MONTHS);
                        }
                        else if (n_sel & 2) {
                            base._scr_sub.screen_change_request(_SCRN_MGR::SUBS_LIST_NODES);
                        }
                    },
                    [&](int idx_sel, TWE_ListView::pair_type&& item_sel) {
                        base._node.year = v_years[idx_sel];
                    }
                )) c = -1;

                // unhandled events
                if (c == -1);
                else if (KeyInput::is_mouse_right_up(c) || c == KeyInput::KEY_LEFT) {
                    base._scr_sub.screen_change_request(_SCRN_MGR::SUBS_LIST_NODES);
                }
                else if (c == KeyInput::KEY_RIGHT) {
                    base._scr_sub.screen_change_request(_SCRN_MGR::SUBS_LIST_MONTHS);
                }
                else switch (c) {
                case KeyInput::KEY_BUTTON_A: break;
                case KeyInput::KEY_BUTTON_B: break;
                case KeyInput::KEY_BUTTON_C: break;
                case KeyInput::KEY_BUTTON_A_LONG: break;
                case KeyInput::KEY_BUTTON_B_LONG: break;
                case KeyInput::KEY_BUTTON_C_LONG: break;
                default: break;
                }
            } while (the_keyboard.available());
        }

        void on_close() {
            btns.clear();
        }

        subscr_list_years(SCR_WSNS_DB& base)
            : B_subs_view_gen()
            , APP_HANDLR_DC(CLS_ID)
            , base(base)
            , btns(*this, base.the_screen)
        {}

        virtual ~subscr_list_years() {}
    };

    /**
     * query months, which node SID has data in the specified year.
     */
    struct subscr_list_months : public B_subs_view_gen, public APP_HANDLR_DC {
    public:
        static const int CLS_ID = _SCRN_MGR::SUBS_LIST_MONTHS; // APP_HANDLER

        SCR_WSNS_DB& base;
        TWE_WidSet_Buttons btns;

        SimpleBuffer<uint32_t> v_months;

        /**
         * update the list view (query months data from DB).
         *
         * \param db
         * \return
         */
        bool update_list(WSnsDb& db) {
            auto& node = base._node;

            lv.clear();
            lv.set_info_area(L">>", L"<<");

            db.query_recorded_months_of_sensor_data(node.sid, node.year,
                [&](int16_t month) {
                    v_months.push_back(uint32_t(month));

                    SmplBuf_WCharSL<64> lbl;
                    lbl << format("%04d/%02d", node.year, month);

                    lv.push_back(lbl.c_str());
                }
            );

            the_keyboard.push(KeyInput::KEY_DOWN); // select first item

            return true;
        }

        void setup() {
            auto& scr = base.the_screen;

            scr.clear();

            update_list(*base._db);
            lv_attach_scr(scr, 3);

            btns.clear();

            const wchar_t* lbl_back = L"[<<年選択]";
            btns.add(scr.get_cols() - TWEUTILS::strlen_vis(lbl_back) - 1, 0, lbl_back,
                [&](int, uint32_t) { base._scr_sub.screen_change_request(_SCRN_MGR::SUBS_LIST_YEARS); });

            // show empty graph
            base._view._clear();
            base._view.plot_empty();

            // title
            base.set_title(L"月選択");
        }

        void loop() {
            auto& scr = base.the_screen;

            btns.check_events();

            // keyboard handling.
            do {
                int c = the_keyboard.read();

                // handle listView events (it needs to pass event even c == -1 to handle timeout)
                if (lv_handle_events(c,
                    [&](int idx_sel, TWE_ListView::pair_type&& item_sel) {
                        base._node.month = v_months[idx_sel];
                        base._scr_sub.screen_change_request(_SCRN_MGR::SUBS_LIST_DAYS);
                    },
                    [&](int n_sel, int idx_sel, TWE_ListView::pair_type&& item_sel) {
                        base._node.month = v_months[idx_sel];
                        if (n_sel & 1) {
                            base._scr_sub.screen_change_request(_SCRN_MGR::SUBS_LIST_DAYS);
                        }
                        else if (n_sel & 2) {
                            base._scr_sub.screen_change_request(_SCRN_MGR::SUBS_LIST_YEARS);
                        }
                    },
                    [&](int idx_sel, TWE_ListView::pair_type&& item_sel) {
                        base._node.month = v_months[idx_sel];
                    }
                )) c = -1;

                // unhandled events
                if (c == -1);
                else if (KeyInput::is_mouse_right_up(c) || c == KeyInput::KEY_LEFT) {
                    base._scr_sub.screen_change_request(_SCRN_MGR::SUBS_LIST_YEARS);
                }
                else if (c == KeyInput::KEY_RIGHT) {
                    base._scr_sub.screen_change_request(_SCRN_MGR::SUBS_LIST_DAYS);
                }
                else switch (c) {
                case KeyInput::KEY_BUTTON_A: break;
                case KeyInput::KEY_BUTTON_B: break;
                case KeyInput::KEY_BUTTON_C: break;
                case KeyInput::KEY_BUTTON_A_LONG: break;
                case KeyInput::KEY_BUTTON_B_LONG: break;
                case KeyInput::KEY_BUTTON_C_LONG: break;
                default: break;
                }

            } while (the_keyboard.available());
        }

        void on_close() {
            btns.clear();
        }

        subscr_list_months(SCR_WSNS_DB& base)
            : B_subs_view_gen()
            , APP_HANDLR_DC(CLS_ID)
            , base(base)
            , btns(*this, base.the_screen)
        {}

        virtual ~subscr_list_months() {}
    };

    /**
     * query days, which node SID has data in the specified year, month.
     */
    struct subscr_list_days : public B_subs_view_gen, public APP_HANDLR_DC {
    public:
        static const int CLS_ID = _SCRN_MGR::SUBS_LIST_DAYS; // APP_HANDLER

        SCR_WSNS_DB& base;
        TWE_WidSet_Buttons btns;

        SimpleBuffer<uint32_t> v_days;

        int32_t day_plotted;

        bool update_list(WSnsDb& db) {
            auto& node = base._node;

            lv.clear();
            lv.set_info_area(L">>", L"<<");

            db.query_recorded_days_of_sensor_data(node.sid, node.year, node.month,
                [&](int16_t day) {
                    v_days.push_back(uint32_t(day));

                    SmplBuf_WCharSL<64> lbl;
                    lbl << format("%04d/%02d/%02d", node.year, node.month, day);

                    lv.push_back(lbl.c_str());
                }
            );

            the_keyboard.push(KeyInput::KEY_DOWN); // select first item

            return true;
        }

        void query_sensor_data_and_plot() {
            auto& node = base._node;
            auto& view = base._view;

            view.plot_day_preview(node.sid, node.year, node.month, node.day);

            day_plotted = node.day;
        }

        void setup() {
            auto& scr = base.the_screen;

            scr.clear();

            update_list(*base._db);
            lv_attach_scr(scr, 3);

            btns.clear();

            const wchar_t* lbl_back = L"[<<月選択]";
            btns.add(scr.get_cols() - TWEUTILS::strlen_vis(lbl_back) - 1, 0, lbl_back,
                [&](int, uint32_t) { base._scr_sub.screen_change_request(_SCRN_MGR::SUBS_LIST_MONTHS); });


            base.set_title(L"日選択");
        }

        void loop() {
            auto& scr = base.the_screen;

            btns.check_events();

            // keyboard handling.
            do {
                auto& node = base._node;
                int c = the_keyboard.read();

                // handle listView events (it needs to pass event even c == -1 to handle timeout)
                if (lv_handle_events(c,
                    [&](int idx_sel, TWE_ListView::pair_type&& item_sel) {
                        node.day = v_days[idx_sel];
                        base._scr_sub.screen_change_request(_SCRN_MGR::SUBS_D_DAY);
                    },
                    [&](int n_sel, int idx_sel, TWE_ListView::pair_type&& item_sel) {
                        node.day = v_days[idx_sel];
                        // next screen
                        if (n_sel & 1) {
                            base._scr_sub.screen_change_request(_SCRN_MGR::SUBS_D_DAY);
                        }
                        else if (n_sel & 2) {
                            base._scr_sub.screen_change_request(_SCRN_MGR::SUBS_LIST_MONTHS);
                        }
                    },
                        [&](int idx_sel, TWE_ListView::pair_type&& item_sel) {
                        node.day = v_days[idx_sel];
                        over_item(idx_sel);
                    }
                    )) c = -1;

                // unhandled events
                if (c == -1);
                else if (KeyInput::is_mouse_left_down(c)) {
                    TWECUI::KeyInput::_MOUSE_EV ev(c);

                    int x = ev.get_x();
                    int y = ev.get_y();

                    if (ev.is_left_btn() && base._view.is_in_area(base._view.area_draw, x, y)) {
                        base._scr_sub.screen_change_request(_SCRN_MGR::SUBS_D_DAY);
                    }
                }
                else if (KeyInput::is_mouse_right_up(c)) {
                    base._scr_sub.screen_change_request(_SCRN_MGR::SUBS_LIST_MONTHS);
                }
                else switch (c) {
                case KeyInput::KEY_LEFT: base._scr_sub.screen_change_request(_SCRN_MGR::SUBS_LIST_MONTHS); break;
                case KeyInput::KEY_RIGHT: base._scr_sub.screen_change_request(_SCRN_MGR::SUBS_D_DAY); break;
                case KeyInput::KEY_BUTTON_A: break;
                case KeyInput::KEY_BUTTON_B: break;
                case KeyInput::KEY_BUTTON_C: break;
                case KeyInput::KEY_BUTTON_A_LONG: break;
                case KeyInput::KEY_BUTTON_B_LONG: break;
                case KeyInput::KEY_BUTTON_C_LONG: break;
                default: break;
                }
            } while (the_keyboard.available());

            if (is_timeout_over_on_item()) {
                auto& node = base._node;

                if (node.day != day_plotted) {
                    query_sensor_data_and_plot();
                }
            }
        }

        void on_close() {
            btns.clear();
        }

        subscr_list_days(SCR_WSNS_DB& base)
            : B_subs_view_gen()
            , APP_HANDLR_DC(CLS_ID)
            , base(base)
            , btns(*this, base.the_screen)
            , day_plotted(-1)
        {}

        virtual ~subscr_list_days() {}
    };

    /**
     * view of the 24H data.
     */
    struct subscr_24hrs : public APP_HANDLR_DC {
    public:
        static const int CLS_ID = _SCRN_MGR::SUBS_D_DAY; // APP_HANDLER

        SCR_WSNS_DB& base;
        TWE_WidSet_Buttons btns;
        struct _btns_id {
            int latest;
            int oldest;
            int next;
            int prev;
        } btns_id;

        SimpleBuffer<uint32_t> v_days;

        //int32_t day_plotted;

        TWESYS::TweLocalTime tl_opened;

        /**
         * query sensor data from DB and plot them.
         * 
         */
        void query_sensor_data_and_plot() {
            WSnsDb& db = *base._db;
            auto& node = base._node;
            auto& view = base._view;

            TWESYS::TweLocalTime t;
            t.year = node.year;
            t.month = node.month;
            t.day = node.day;
            t.hour = 0;
            t.minute = 0;
            t.second = 0;
            t.get_epoch();

            // init view values
            view.full_cursor_idx = -1;

            view.set_start_epoch_and_duration(t.epoch, 86400);
            view.clear_full();

            uint32_t ct = 0, t0 = millis();
            // db.query_sensor_data_by_day(node.sid, node.year, node.month, node.day, 
            db.query_sensor_data(node.sid, t.epoch, t.epoch + 86399,
                [&](WSnsDb::SENSOR_DATA& d) {
                    view.add_entry_full(d);
                    ct++;
                }
            );
            uint32_t t1 = millis();
            base.the_screen_b << crlf << format("<%d:Q_dday=%d ct=%d t=%d>", t1, node.day, ct, t1 - t0);

            view.preudo_reload();
            view.plot_graph();
            
            // show essential information
            auto& scr = base.the_screen;
            base.set_title(L"24時間データ");

            int l = 1;
            l++;
            view.disp_sid_text(node.sid, l); l++;
            scr(0, l++) << "  SID: " << format("%08X", node.sid);
            l++;
            scr(0, l) << " DATE: ";
            scr(10, l) << format("%04d/%02d/%02d", node.year, node.month, node.day);

            // set plot
            //day_plotted = node.day;

            // buttons
            btns[btns_id.next].set_visible(node.ts_newest && *node.ts_newest >= DB_TIMESTAMP::value_type(t.epoch + 86400));
            btns[btns_id.latest].set_visible(node.ts_newest && *node.ts_newest >= DB_TIMESTAMP::value_type(t.epoch + 86400));
            btns[btns_id.prev].set_visible(node.ts_oldest && *node.ts_oldest < DB_TIMESTAMP::value_type(t.epoch));
        }

        void setup() {
            WSnsDb& db = *base._db;
            auto& scr = base.the_screen;
            auto& node = base._node;
            auto& view = base._view;

            tl_opened.now();

            scr.clear();

            btns.clear();
            const wchar_t* lbl_back = L"[<<一覧]";
            btns.add(scr.get_cols() - TWEUTILS::strlen_vis(lbl_back) - 1, 0, lbl_back,
                [&](int, uint32_t) {
                    //node.set_sid(0);
                    base._scr_sub.screen_change_request(_SCRN_MGR::SUBS_LIST_NODES);
                }
            );

            btns_id.prev = btns.add(7, 5, L"<<",
                [&](int, uint32_t) { 
                    DB_TIMESTAMP ts_result;
                    base._db->query_previous_ts(node.sid, view.t_start - 1, ts_result);
                    if (ts_result) {
                        TWESYS::TweLocalTime t;
                        t.set_epoch(*ts_result);
                        
                        node.year = t.year;
                        node.month = t.month;
                        node.day = t.day;
                        node.hour = 0;

                        query_sensor_data_and_plot();
                    }
                }
            );

            btns_id.next = btns.add(21, 5, L">>",
                [&](int, uint32_t) {
                    DB_TIMESTAMP ts_result;
                    base._db->query_next_ts(node.sid, view.t_end, ts_result);
                    if (ts_result) {
                        TWESYS::TweLocalTime t;
                        t.set_epoch(*ts_result);

                        node.year = t.year;
                        node.month = t.month;
                        node.day = t.day;
                        node.hour = 0;

                        query_sensor_data_and_plot();
                    }
                }
            );

            btns_id.latest = btns.add(0, 7, L"[最新]",
                [&](int, uint32_t) {
                    if (node.ts_newest) {
                        TWESYS::TweLocalTime t;
                        t.set_epoch(*node.ts_newest);

                        node.year = t.year;
                        node.month = t.month;
                        node.day = t.day;
                        node.hour = 0;

                        query_sensor_data_and_plot();
                    }
                }
            );

            const wchar_t* lbl_live = L"[ライブ>>]";
            btns.add(scr.get_cols() - strlen_vis(lbl_live) - 1, 9, lbl_live,
                [&](int, uint32_t) {
                    base._scr_sub.screen_change_request(_SCRN_MGR::SUBS_LIVE_VIEW);
                }
            );

            btns.add(7, 7, L"[年]",
                [&](int, uint32_t) { base._scr_sub.screen_change_request(_SCRN_MGR::SUBS_LIST_YEARS); }
            );

            btns.add(7 + 6, 7, L"[月]",
                [&](int, uint32_t) { base._scr_sub.screen_change_request(_SCRN_MGR::SUBS_LIST_MONTHS); }
            );

            btns.add(7 + 12, 7, L"[日]",
                [&](int, uint32_t) { base._scr_sub.screen_change_request(_SCRN_MGR::SUBS_LIST_DAYS); }
            );

            btns.add(1, 11, L"[CSV出力]",
                [&](int, uint32_t) { base.export_sensor_data_day(uint32_t(node.sid), node.year, node.month, node.day); }
            );

            
            // the date must be set in advance, just in case if it's not.
            if (!node.has_set_day()) {
                node.set_latest_date();
            }

            query_sensor_data_and_plot();

            
            // show help message
            // l = scr.get_rows() - 1; // last line
        }

        void loop() {
            auto& scr = base.the_screen;
            auto& view = base._view;
            auto& node = base._node;

            btns.check_events();

            // keyboard handling.
            do {
                auto& node = base._node;
                int c = the_keyboard.read();

                // unhandled events
                if (c == -1);
                else if (KeyInput::is_mouse_left_down(c)) {
                    TWECUI::KeyInput::_MOUSE_EV ev(c);

                    int x = ev.get_x();
                    int y = ev.get_y();

                    if (ev.is_left_btn() && view.is_in_area(view.area_draw, x, y)) {
                        if (view.pseudo_scale > 1) {
                            view.drag.x_drag = x;
                            view.drag.y_drag = y;
                            view.drag.view_idx_when_dragged = view.pseudo_start_idx; // save the viewing index when dragging begins.
                            view.drag.b_dragging = true;
                            view.drag.b_drag_scrollbar = false;
                            if (view.is_in_area(view.area_scroll_bar, x, y)) {
                                // align dragging step to the scroll bar.
                                view.drag.b_drag_scrollbar = true;
                            }
                        }
                    }
                }
                else if (KeyInput::is_mouse_left_up(c)) {
                    // end of drag operation
                    if (view.drag.b_dragging) {
                        view.drag.b_dragging = false;
                        view.drag.b_drag_scrollbar = false;
                    }
                }
                else if (KeyInput::is_mouse_move(c)) {
                    TWECUI::KeyInput::_MOUSE_EV ev(c);

                    int x = ev.get_x();
                    int y = ev.get_y();

                    // save last coord
                    view.drag.x_last = x;
                    view.drag.y_last = y;

                    bool b_redraw = false;
                    
                    // check
                    if (   view.v_dat_full.length() == _VIEW::DATA_WIDTH
                        && (view.is_in_area(view.area_graph, x, y) || view.is_in_area(view.area_graph_sub, x, y))
                       )
                    {
                        if (view.pick_sample_and_display_info(x, false) != -1) b_redraw = true;
                    }

                    // update view
                    if (view.drag.b_dragging) {
                        // if scroll bar is dragged, set opposite dir and scale by magnified factor.
                        int32_t x_scrbar = view.drag.b_drag_scrollbar ? -view.pseudo_scale : 1; 
                        
                        // set new index
                        view.pseudo_start_idx = view.drag.view_idx_when_dragged + (view.drag.x_drag - x) * x_scrbar; 

                        // limit start index range
                        if (view.pseudo_start_idx < 0) view.pseudo_start_idx = 0;
                        if (view.pseudo_start_idx > view.pseudo_width - _VIEW::DRAW_WIDTH) view.pseudo_start_idx = view.pseudo_width - _VIEW::DRAW_WIDTH;
                        
                        b_redraw = true;
                    }

                    view.set_pointer_pos(x, y);

                    if (view.is_in_area(view.area_draw, x, y)) {
                        view.plot_graph(); // if(b_redraw)
                    }
                    else {
                        view.plot_cursor_control(1); // show
                    }
                }
                else if (KeyInput::is_mouse_wheel(c)) {
                    if (view.is_in_area(view.area_draw, view.drag.x_last, view.drag.y_last)) {
                        TWECUI::KeyInput::MOUSE_WHEEL ev(c);

                        // calculate relative x position from last mouse coord.
                        int32_t x_rel = view.drag.x_last - view.area_graph.x;
                        if (x_rel < 0) x_rel = 0;
                        if (x_rel >= _VIEW::DRAW_WIDTH) x_rel = _VIEW::DRAW_WIDTH - 1;

                        // check
                        if (view.v_dat_full.length() == _VIEW::DATA_WIDTH
                            && (view.is_in_area(view.area_graph, view.drag.x_last, view.drag.y_last) || view.is_in_area(view.area_graph_sub, view.drag.x_last, view.drag.y_last))
                            )
                        {
                            view.pick_sample_and_display_info(view.drag.x_last, false);
                        }

                        if (ev.get_y() > 0) {
                            // scroll down
                            if (view.pseudo_scale_down(x_rel)) {
                                view.plot_graph();
                            }
                        }
                        else if (ev.get_y() < 0) {
                            // scroll up
                            if (view.pseudo_scale_up(x_rel)) {
                                view.plot_graph();
                            }
                        }
                    }
                }
                else if (KeyInput::is_mouse_right_up(c) ) {
                    base._scr_sub.screen_change_request(_SCRN_MGR::SUBS_PREVIOUS);
                }
                else switch (c) {
                case KeyInput::KEY_LEFT: base._view.pick_sample_prev(false); break;
                case KeyInput::KEY_RIGHT: base._view.pick_sample_next(false); break;
                case KeyInput::KEY_UP: base._view.pseudo_scale_up(); view.plot_graph(); break;
                case KeyInput::KEY_DOWN: base._view.pseudo_scale_down(); view.plot_graph(); break;
                case KeyInput::KEY_BUTTON_A: break;
                case KeyInput::KEY_BUTTON_B: break;
                case KeyInput::KEY_BUTTON_C: break;
                case KeyInput::KEY_BUTTON_A_LONG: break;
                case KeyInput::KEY_BUTTON_B_LONG: break;
                case KeyInput::KEY_BUTTON_C_LONG: break;
                default: break;
                }
            } while (the_keyboard.available());

            // if updated data.
            if (base._node.b_live_update) {
                base._node.b_live_update = false;

                // recreate viewing data v_dat[]
                view.preudo_reload();
                view.plot_graph();
            }

            // check new day and update screen (if new day comes, redraw or move next day)
            if (base._lt_now.day != tl_opened.day) {
                tl_opened = base._lt_now; // update opened day.

                TWESYS::TweLocalTime tl_v;
                tl_v.year = node.year;
                tl_v.month = node.month;
                tl_v.day = node.day;
                tl_v.get_epoch();
                tl_v.set_epoch(tl_v.epoch + 86400); // set next day from the view.

                // last view is on yesterday, move on tomorrow.
                if (base._lt_now.day == tl_v.day) {
                    node.year = base._lt_now.year;
                    node.month = base._lt_now.month;
                    node.day = base._lt_now.day;
                    node.hour = 0;
                } 

                // update view
                query_sensor_data_and_plot();
            }
        }

        void on_close() {
            btns.clear();
            screen_show_cursor();
        }

        subscr_24hrs(SCR_WSNS_DB& base)
            : APP_HANDLR_DC(CLS_ID)
            , base(base)
            , btns(*this, base.the_screen), btns_id()
            //, day_plotted(-1)
        {}

        virtual ~subscr_24hrs() {}
    };

    /**
     * debug screen
     */
    struct subscr_fatal : APP_HANDLR_DC {
        static const int CLS_ID = _SCRN_MGR::SUBS_FATAL;

        SCR_WSNS_DB& base;

        TWE_WidSet_Buttons btns;
        uint32_t t_screen_opened;

        void go_back() {
            the_app.exit(APP_ID);
        }
        
        void setup() {
            auto& scr = base.the_screen;

            t_screen_opened = millis();

            int c = 0, l = 4;
            scr.clear();
            scr(c, l++) << "*** データベースの初期";
            scr(c, l++) << "    化に失敗しました。";
            scr(c, l++) << "->直前の画面に戻ります。";

            l++;
            btns.add(0, l++, L"[戻る]", [&](int, uint32_t) { go_back(); });
        }

        void loop() {
            auto& scr = base.the_screen;

            btns.check_events();

            // timeout in 10sec
            if (millis() - t_screen_opened > 10000) {
                go_back();
                return;
            }

            // keyboard handling.
            do {
                int c = the_keyboard.read();

                // unhandled events
                switch (c) {
                case -1: break;

                case KeyInput::KEY_ENTER:
                    go_back();
                    break;

                case KeyInput::KEY_BUTTON_A:
                    break;

                case KeyInput::KEY_BUTTON_B:
                    break;
                }

            } while (the_keyboard.available());
        }

        void on_close() {
            btns.clear();
        }

        subscr_fatal(SCR_WSNS_DB& base)
            : base(base)
            , btns(*this, base.the_screen)
            , APP_HANDLR_DC(CLS_ID)
            , t_screen_opened(0)
        {}

        virtual ~subscr_fatal() {}
    };

#ifdef WSNS_DEBUG_FUNCTIONS
    /**
     * debug screen
     */
    struct subscr_debug : APP_HANDLR_DC {
        static const int CLS_ID = _SCRN_MGR::SUBS_DEBUG;
        
        SCR_WSNS_DB& base;
        TWE_WidSet_Buttons btns;

        void query_table_column() {
            auto& db = *base._db;

            try {
                auto query = db.sql_statement(
                    "PRAGMA table_info(\"sensor_data\");"
                );

                while (query.executeStep()) {
                    std::cerr << query.getColumn(1).getString() << std::endl;
                }
            }
            catch (std::exception& e)
            {
                (void)e;
            }
        }

        void setup() {
            auto& scr = base.the_screen;

            scr.clear();
            scr(0, 0) << "screen message only.";

            btns.clear();
            btns.add(0, 1, L"[Brows nodes]", [&](int,uint32_t) { base._scr_sub.screen_change_request(_SCRN_MGR::SUBS_LIST_NODES); });

            btns.add(0, scr.get_rows() - 5, L"[GetColNames]", [&](int, uint32_t) { query_table_column(); });
            btns.add(0, scr.get_rows() - 4, L"[add dummy 10]", [&](int, uint32_t) { base.db_insert_dummy_entries(10ul, 60); });
            btns.add(0, scr.get_rows() - 3, L"[add dummy 1M]", [&](int, uint32_t) { base.db_insert_dummy_entries(1000000ul, 86400ul * 365 * 3); });
            btns.add(0, scr.get_rows() - 2, L"[add dummy 10M]", [&](int, uint32_t) { base.db_insert_dummy_entries(10000000ul, 86400ul * 365 * 3); });
            btns.add(0, scr.get_rows() - 1, L"[add dummy 100M]", [&](int, uint32_t) { base.db_insert_dummy_entries(100000000ul, 86400ul * 365 * 3); });
        }

        void loop() {
            auto& scr = base.the_screen;

            btns.check_events();

            // keyboard handling.
            do {
                int c = the_keyboard.read();

                // unhandled events
                switch (c) {
                case -1: break;

                case KeyInput::KEY_ENTER:
                    base._scr_sub.screen_change_request(_SCRN_MGR::SUBS_LIST_NODES);
                    break;

                case KeyInput::KEY_BUTTON_A:
                    break;

                case KeyInput::KEY_BUTTON_B:
                    break;
                }

            } while (the_keyboard.available());
        }

        void on_close () {
            btns.clear();
            //uptr_screen.reset(nullptr);
        }

        subscr_debug(SCR_WSNS_DB& base)
            : base(base)
            , btns(*this, base.the_screen)
            , APP_HANDLR_DC(CLS_ID)
        {}

        virtual ~subscr_debug() {}
    };
#endif
    
    /**
     * perform CSV export.
     * -all data queried by specified SID/Year/Month/Day is exported.
     * -basically, all records of sensor_data table is exported, but
     *  time format (YYYY/MM/DD HH:MM:DD) is added.
     * 
     * \param sid       the SID
     * \param year      year
     * \param month     month
     * \param day       day
     * \return 
     */
    bool export_sensor_data_day(uint32_t sid, int16_t year, int16_t month, int16_t day) {
        if (!_db) return false;
        
        WSnsDb& db = *_db;

        SmplBuf_ByteSL<128> fname;
        fname << WSNS_EXPORT_FILENAME
              << format("%08X_%04d-%02d-%02d", sid, year, month, day);

        // open log file
        TweLogFile logf(fname.c_str(), WSNS_EXPORT_FILEEXT, false);

        // query
        TWESYS::TweLocalTime t;
        t.year = year;
        t.month = month;
        t.day = day;
        t.hour = 0;
        t.minute = 0;
        t.second = 0;
        t.get_epoch();

        if (logf.open(false)) {
            try {
                try {
                    auto query = db.sql_statement(
                        "PRAGMA table_info(\"sensor_data\");"
                    );

                    int ct = 0;
                    while (query.executeStep()) {
                        if (ct == 0) {
                            logf.os() << "ct";
                        }
                        else if (ct == 2) {
                            logf.os() << "," << "ts";
                            logf.os() << "," << "date";
                        }
                        else {
                            logf.os() << "," << query.getColumn(1).getText();
                        }
                        ct++;
                    }

                    logf.os() << std::endl;
                }
                catch (std::exception& e) {
                    (void)e;
                }

                try {
                    auto query = db.sql_statement("SELECT * FROM sensor_data WHERE (sid=?) and (ts BETWEEN ? and ?) ORDER BY ts ASC"  // sorted 
                        , DB_INTEGER(int32_t(sid))
                        , DB_TIMESTAMP(t.epoch)
                        , DB_TIMESTAMP(t.epoch + 86400 - 1)
                    );

                    SmplBuf_ByteS line(4095);

                    int ct = 1;
                    while (query.executeStep()) {
                        line << format("%d", ct);
                        for (int i = 1; i < query.getColumnCount(); i++) {
                            line << uint8_t(',');
                            const auto& c = query.getColumn(i);
                            if (i == 1) { // must be SID
                                line << format("0x%08X", c.getInt());
                            } 
                            else if (i == 2) { // must be UNIX epoch
                                line << format("%lld,", c.getInt64());

                                // for excel (convert unix epoch is a bit annoying)
                                int sec = int(c.getInt64() - t.epoch);
                                line << format("%04d/%02d/%02d %02d:%02d:%02d", year, month, day, sec / 3600, (sec % 3600) / 60, sec % 60);
                            }
                            else if (c.isInteger()) {
                                line << format("%d", c.getInt());
                            }
                            else if (c.isFloat()) {
                                double d = c.getDouble();

                                double d_abs = std::abs(d);
                                if (d_abs > 0.0) d_abs += 0.0000005;

                                // output a sign
                                if (d < 0) line << '-';
                                
                                // split into int part and frac part.
                                double d_int;
                                double d_frac = std::modf(d_abs, &d_int);
                                
                                // prepare integer presentation of int and frac part.
                                int n_frac = int(d_frac * 1000000.);
                                int n_int = d_abs < INT_MAX ? (int)d_int : INT_MAX; // if too large, set INT_MAX value.

                                if (d_abs == 0.0) line << "0.0";
                                else if (d_abs < 0.001) line << format("%e", d_frac);
                                else if (d_abs < 1) line << format("%1.6f", d_frac);
                                else if (n_int < 1000) line << format("%d.%03d", n_int, n_frac / 1000);
                                else if (n_int < 10000) line << format("%d.%02d", n_int, n_frac / 10000);
                                else if (n_int < 100000) line << format("%d.%01d", n_int, n_frac / 100000);
                                else if (n_int < 10000000) line << format("%d", n_int);
                                else line << format("%e", d_abs);
                            }
                            else {
                                ; // print nothing
                            }
                        }
                        ct++;

                        logf.os() << line.c_str() << std::endl;
                        line.clear();
                    }

                    logf.close();
                    logf.shell_open();
                    return true;
                }
                catch (std::exception& e) {
                    std::cerr << std::endl << "SCR_WSNS_DB::export_sensor_data_day(): " << e.what();
                    return false;
                }
            }
            catch (std::ios_base::failure &e) {
                std::cerr << std::endl << "SCR_WSNS_DB::export_sensor_data_day(): " << e.what();
                return false;
            }
        }
        
        return false;
    }

    /**
     * the setup.
     */
	void setup() {
        // main area
        the_screen.clear_screen();
        the_screen.force_refresh(); // force clear first

        // save size of main area
        const int WIDTH_MAINSCR = 144;
        const int MARGIN = 6;
        _area_scr_main = the_screen.get_draw_area(); // save the area of the_screen.

        // define the area of the graph
        _area_draw = _area_scr_main;
        _area_draw.w -= (WIDTH_MAINSCR + MARGIN); // make a space of 128pix on the right for the main screen.

        // move the main screen and clear
        the_screen.set_draw_area({ 640 - WIDTH_MAINSCR, _area_scr_main.y, WIDTH_MAINSCR, _area_scr_main.h });
        the_screen.set_font(_app.font_IDs.smaller); // use font ID (small font)
        the_screen.clear_screen();
        the_screen.force_refresh();

        // other screen area
		the_screen_b.clear_screen();
		_app.set_title_bar(PAGE_ID::PAGE_BASIC);
		_app.set_nav_bar();

        // time stamp
        _lt_now.now();

        // open DB
        if (db_open()) {
            // query nodes list
            _db->query_sorted_sensor_list_newer_first([&](uint32_t sid, uint64_t ts) {}); // load SID list.
            _db->sensor_node_check(); // if missing node desc in sensor_node table, insert dummy one.

            // view setting
            _view.set_render_area(_area_draw);

            // open list view first
            _scr_sub.screen_change_request(_SCRN_MGR::SUBS_LIST_NODES);

#ifdef WSNS_DEBUG_FUNCTIONS
            // prepare random functions
            _rnd.setup();
#endif        
        }
        else {
            _scr_sub.screen_change_request(_SCRN_MGR::SUBS_FATAL);
        }
	}

	void loop() {
        // read the uart queue
        do {
            int c = the_uart_queue.read();

            if (c >= 0) parse_a_byte(c);

        } while (the_uart_queue.available());

        // DB handling (commit every seconds)
        if (_is_new_sec()) {
            if ((_sec % WSNS_DB_COMMIT_PERIOD) == 1) {
                //WrtCon << crlf << format("[%d]", _sec);
                db_commit();
            }

            _lt_now.now();
        }

        // buttons events handling
        _btns.check_events();

        // call loop of subscreens
        APP_HNDLR<SCR_WSNS_DB>::loop();
        
        // subscreen change request (this should be done at the end of loop(), because deleting instance may cause an access violation.)
        _scr_sub.screen_change_perform(*this);
	}

	void on_close() {
        APP_HNDLR<SCR_WSNS_DB>::on_close();

        the_screen.set_draw_area(_area_scr_main);
        the_screen.set_font(_app.font_IDs.main);
        the_screen.clear_screen();
        the_screen.force_refresh();
	}

    /**
     * the constructor.
     * 
     * \param app       base application object (App_CUE)
     */
    SCR_WSNS_DB(App_CUE& app)
        : _app(app)
        , APP_HANDLR_DC(CLS_ID)
        , _btns(*this, app.the_screen)
        , _pkt_rcv_ct(0)
        , the_screen(app.the_screen), the_screen_b(app.the_screen_b), parse_ascii(app.parse_ascii)
        , _db(), _db_transaction()
        , _sec(0)
        , _scr_sub()
        , _node(*this)
        , _area_draw(), _area_scr_main()
        , _view(*this)
#ifdef WSNS_DEBUG_FUNCTIONS
        , _rnd()
#endif
    {
        ;
    }

    /**
     * the destructor.
     */
    ~SCR_WSNS_DB()
    {
        db_close();
    }
};

// generate handler instance (SCR_XXX needs to have setup(), loop(), on_close() methods)
void App_CUE::hndr_SCR_WSNS_DB(event_type ev, arg_type arg) { hndr<SCR_WSNS_DB>(ev, arg); }
