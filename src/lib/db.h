#ifndef K_DB_H_
#define K_DB_H_

namespace K {
  class DB {
    public:
      static void main(Local<Object> exports) {
        dbFpath = string("/data/db/K.").append(to_string((int)CF::cfExchange())).append(".").append(to_string(CF::cfBase())).append(".").append(to_string(CF::cfQuote())).append(".db");
        if (sqlite3_open(dbFpath, &db)) { cout << FN::uiT() << sqlite3_errmsg(db) << endl; exit(1); }
        cout << FN::uiT() << "DB " << dbFpath << " loaded OK." << endl;
        NODE_SET_METHOD(exports, "dbLoad", DB::_load);
        NODE_SET_METHOD(exports, "dbInsert", DB::_insert);
      };
      static json load(uiTXT k) {
        return load(string(1, (char)k));
      };
      static json load(string k) {
        char* zErrMsg = 0;
        sqlite3_exec(db,
          string("CREATE TABLE ").append(k).append("("                 \
          "id    INTEGER  PRIMARY KEY  AUTOINCREMENT        NOT NULL," \
          "json  BLOB                                       NOT NULL," \
          "time  TIMESTAMP DEFAULT (CAST((julianday('now') - 2440587.5)*86400000 AS INTEGER))  NOT NULL);"),
          NULL, NULL, &zErrMsg
        );
        string j = "[";
        sqlite3_exec(db,
          string("SELECT json FROM ").append(k).append(" ORDER BY time DESC;"),
          cb, (void*)&j, &zErrMsg
        );
        if (zErrMsg) printf("sqlite error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        if (j[strlen(j.data()) - 1] == ',') j.pop_back();
        return json::parse(j.append("]"));
      };
      static void insert(uiTXT k, Local<Object> o, bool rm = true, string id = "NULL", long time = 0) {
        Isolate* isolate = Isolate::GetCurrent();
        char* zErrMsg = 0;
        string r;
        MaybeLocal<Array> maybe_props = o->GetOwnPropertyNames(Context::New(isolate));
        if (!maybe_props.IsEmpty()) {
          JSON Json;
          MaybeLocal<String> r_ = Json.Stringify(isolate->GetCurrentContext(), o);
          r = FN::S8v(r_.ToLocalChecked());
        } else r = "";
        sqlite3_exec(db,
          string((rm || id != "NULL" || time) ? string("DELETE FROM ").append(string(1, (char)k))
          .append(id != "NULL" ? string(" WHERE id = ").append(id).append(";") : (
            time ? string(" WHERE time < ").append(to_string(time)).append(";") : ";"
          ) ) : "").append((!r.length() || r == "{}") ? "" : string("INSERT INTO ")
            .append(string(1, (char)k)).append(" (id,json) VALUES(").append(id).append(",'")
            .append(r).append("');")),
          NULL, NULL, &zErrMsg
        );
        if (zErrMsg) printf("sqlite error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
      };
    private:
      static void _load(const FunctionCallbackInfo<Value>& args) {
        Isolate* isolate = args.GetIsolate();
        HandleScope scope(isolate);
        json k = load(FN::S8v(args[0]->ToString()));
        JSON Json;
        MaybeLocal<Value> array = Json.Parse(isolate->GetCurrentContext(), FN::v8S(k.dump().data()));
        args.GetReturnValue().Set(array.IsEmpty() ? (Local<Value>)Array::New(isolate) : array.ToLocalChecked());
      };
      static void _insert(const FunctionCallbackInfo<Value>& args) {
        Isolate* isolate = args.GetIsolate();
        HandleScope scope(isolate);
        insert((uiTXT)FN::S8v(args[0]->ToString())[0], args[1]->IsUndefined() ? Object::New(isolate) : args[1]->ToObject(), args[2]->IsUndefined() ? true : args[2]->BooleanValue(), args[3]->IsUndefined() ? "NULL" : FN::S8v(args[3]->ToString()), args[4]->IsUndefined() ? 0 : args[4]->NumberValue());
      };
      static int cb(void *param, int argc, char **argv, char **azColName) {
        string* j = reinterpret_cast<string*>(param);
        for (int i=0; i<argc; i++) j->append(argv[i]).append(",");
        return 0;
      };
  };
}

#endif
