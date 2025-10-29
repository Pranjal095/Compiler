#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <string>
#include <unordered_map>
#include <memory>
#include <vector>

enum SymbolKind {
    SYM_VARIABLE,
    SYM_FUNCTION,
    SYM_TYPE,
    SYM_ROLE,
    SYM_TASK
};

struct SymbolInfo {
    std::string name;
    SymbolKind kind;
    std::string type_name;  // For variables: "int", "float", etc.
    int array_size;         // -1 if not an array
    int role_size;          // For roles
    bool is_array;
    bool is_initialized;
    int line_declared;
    
    SymbolInfo(const std::string& n, SymbolKind k, const std::string& t = "", int line = 0)
        : name(n), kind(k), type_name(t), array_size(-1), role_size(-1), 
          is_array(false), is_initialized(false), line_declared(line) {}
};

class SymbolTable {
private:
    std::unordered_map<std::string, std::unique_ptr<SymbolInfo>> symbols;
    SymbolTable* parent;
    std::vector<std::unique_ptr<SymbolTable>> children;
    int scope_level;
    std::string scope_name;

public:
    SymbolTable(SymbolTable* parent = nullptr, const std::string& name = "global");
    ~SymbolTable();
    
    // Symbol management
    bool insert(const std::string& name, SymbolKind kind, const std::string& type = "", int line = 0);
    SymbolInfo* lookup(const std::string& name);
    SymbolInfo* lookup_current_scope(const std::string& name);
    
    // Scope management
    SymbolTable* create_child_scope(const std::string& name);
    SymbolTable* get_parent() { return parent; }
    int get_scope_level() const { return scope_level; }
    
    // Display
    void print(int indent = 0) const;
    void print_symbol(const SymbolInfo* sym) const;
};

#endif