#ifndef DEADLOCK_ANALYZER_H
#define DEADLOCK_ANALYZER_H

#include "../AST/astNodes.h"
#include <map>
#include <set>
#include <vector>
#include <string>

// Represents a communication event in the program
struct CommEvent {
    enum Type { SEND, RECV, BROADCAST, GATHER, SPAWN };
    Type type;
    std::string from_role;      // Role performing the communication
    std::string to_role;        // Target role
    std::string task_name;      // Task containing this event
    int line_number;            // For error reporting (future enhancement)
    
    CommEvent(Type t, const std::string& from, const std::string& to, const std::string& task) : type(t), from_role(from), to_role(to), task_name(task), line_number(0) {}
};

// Represents a potential deadlock scenario
struct DeadlockWarning {
    std::string type;           // circular_wait, send_recv_mismatch, so on
    std::string description;
    std::vector<std::string> involved_roles;
    std::vector<std::string> involved_tasks;
};

class DeadlockAnalyzer {
public:
    DeadlockAnalyzer();
    
    // Main entry point, analyze the entire program
    void analyzeProgram(Program* program);
    
    // Check if deadlocks were detected
    bool hasDeadlocks() const { return !warnings.empty(); }
    
    // Print all warnings to stderr
    void printWarnings() const;
    
private:
    // Program metadata
    std::map<std::string, std::string> taskToRole;  // task_name to role_name mapping
    std::map<std::string, int> roleSizes;            // role_name to array size mapping
    std::set<std::string> spawnedRoles;             // roles that are spawned
    
    // Communication events
    std::vector<CommEvent> commEvents;
    
    // Detected warnings
    std::vector<DeadlockWarning> warnings;
    
    // Analysis passes
    void extractMetadata(Program* program);
    void extractCommEvents(Program* program);
    void extractTaskCommEvents(TaskDecl* task, const std::string& roleName);
    void extractStmtCommEvents(Stmt* stmt, const std::string& currentRole, const std::string& taskName);
    
    // Deadlock detection algorithms
    void detectCircularWait();
    void detectSendRecvMismatches();
    void detectGatherSpawnInconsistencies();
    
    // Wait-for graph
    std::map<std::string, std::set<std::string>> waitForGraph;  // role -> set of roles it waits for
    void buildWaitForGraph();
    bool hasCycle(const std::string& start, std::set<std::string>& visited, std::set<std::string>& recStack, std::vector<std::string>& cycle);
    
    // Helper functions
    void addWarning(const std::string& type, const std::string& desc, const std::vector<std::string>& roles, const std::vector<std::string>& tasks);
};

#endif
