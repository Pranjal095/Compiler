#ifndef DEADLOCK_ANALYZER_H
#define DEADLOCK_ANALYZER_H

#include "../AST/astNodes.h"
#include <map>
#include <set>
#include <vector>
#include <string>

// Represents a communication event in the program with execution context
struct CommEvent {
    enum Type { SEND, RECV, BROADCAST, GATHER, SPAWN };
    Type type;
    std::string from_role;      // Role performing the communication
    std::string to_role;        // Target role
    std::string task_name;      // Task containing this event
    std::string context_path;   // Execution path (e.g., "if_branch", "else_branch")
    int line_number;            // For error reporting (future enhancement)
    
    CommEvent(Type t, const std::string& from, const std::string& to, const std::string& task, const std::string& ctx = "") : type(t), from_role(from), to_role(to), task_name(task), context_path(ctx), line_number(0) {}
};

// Represents a potential deadlock scenario
struct DeadlockWarning {
    std::string type;           // circular_wait, send_recv_mismatch, so on
    std::string description;
    std::vector<std::string> involved_roles;
    std::vector<std::string> involved_tasks;
    std::string execution_path; // Which branch caused the deadlock
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
    
    // Communication events (path-sensitive)
    std::vector<CommEvent> commEvents;
    
    // Detected warnings
    std::vector<DeadlockWarning> warnings;
    
    // Analysis passes
    void extractMetadata(Program* program);
    void extractCommEvents(Program* program);
    void extractTaskCommEvents(TaskDecl* task, const std::string& roleName);
    void extractStmtCommEvents(Stmt* stmt, const std::string& currentRole, const std::string& taskName, const std::string& contextPath = "");
    
    // Path-sensitive analysis
    void analyzePathSensitive();
    std::vector<CommEvent> filterEventsByPath(const std::string& path_filter);
    
    // Deadlock detection algorithms
    void detectCircularWait();
    void detectCircularWaitForPath(const std::string& path, const std::vector<CommEvent>& events);
    void detectSendRecvMismatches();
    void detectSendRecvMismatchesForPath(const std::string& path, const std::vector<CommEvent>& events);
    void detectGatherSpawnInconsistencies();
    
    // Wait-for graph
    std::map<std::string, std::set<std::string>> waitForGraph;  // role -> set of roles it waits for
    void buildWaitForGraph();
    void buildWaitForGraphFromEvents(const std::vector<CommEvent>& events);
    bool hasCycle(const std::string& start, std::set<std::string>& visited, std::set<std::string>& recStack, std::vector<std::string>& cycle);
    
    // Helper functions
    void addWarning(const std::string& type, const std::string& desc, const std::vector<std::string>& roles, const std::vector<std::string>& tasks, const std::string& path = "");
    std::set<std::string> getUniquePaths();
};

#endif
