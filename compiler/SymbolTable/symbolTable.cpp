#include "symbolTable.h"
#include <iostream>
#include <iomanip>

SymbolTable::SymbolTable(SymbolTable* parent, const std::string& name)
    : parent(parent), scope_name(name) {
    scope_level = parent ? parent->scope_level + 1 : 0;
}

SymbolTable::~SymbolTable() {
    // unique_ptr will handle cleanup
}

bool SymbolTable::insert(const std::string& name, SymbolKind kind, const std::string& type, int line) {
    // Check if symbol already exists in current scope
    if (symbols.find(name) != symbols.end()) {
        return false;
    }
    
    auto sym = std::make_unique<SymbolInfo>(name, kind, type, line);
    symbols[name] = std::move(sym);
    return true;
}

SymbolInfo* SymbolTable::lookup(const std::string& name) {
    // Search current scope
    auto it = symbols.find(name);
    if (it != symbols.end()) {
        return it->second.get();
    }
    
    // Search parent scopes
    if (parent) {
        return parent->lookup(name);
    }
    
    return nullptr;
}

SymbolInfo* SymbolTable::lookup_current_scope(const std::string& name) {
    auto it = symbols.find(name);
    if (it != symbols.end()) {
        return it->second.get();
    }
    return nullptr;
}

SymbolTable* SymbolTable::create_child_scope(const std::string& name) {
    auto child = std::make_unique<SymbolTable>(this, name);
    SymbolTable* child_ptr = child.get();
    children.push_back(std::move(child));
    return child_ptr;
}

void SymbolTable::print(int indent) const {
    std::string prefix(indent * 2, ' ');
    std::cout << prefix << "Scope: " << scope_name << " (level " << scope_level << ")\n";
    std::cout << prefix << "Symbols:\n";
    
    for (const auto& [name, sym] : symbols) {
        std::cout << prefix << "  ";
        print_symbol(sym.get());
    }
    
    for (const auto& child : children) {
        child->print(indent + 1);
    }
}

void SymbolTable::print_symbol(const SymbolInfo* sym) const {
    std::cout << sym->name << " (";
    
    switch (sym->kind) {
        case SYM_VARIABLE: std::cout << "var"; break;
        case SYM_FUNCTION: std::cout << "func"; break;
        case SYM_TYPE: std::cout << "type"; break;
        case SYM_ROLE: std::cout << "role"; break;
        case SYM_TASK: std::cout << "task"; break;
    }
    
    if (!sym->type_name.empty()) {
        std::cout << ", type: " << sym->type_name;
    }
    
    if (sym->is_array) {
        std::cout << "[" << sym->array_size << "]";
    }
    
    std::cout << ", line: " << sym->line_declared << ")\n";
}
