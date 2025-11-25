#include "deadlock_analyzer.h"
#include <iostream>
#include <algorithm>

DeadlockAnalyzer::DeadlockAnalyzer() {}

void DeadlockAnalyzer::analyzeProgram(Program* program) {
    if (!program) return;
    
    // Clear previous state
    warnings.clear();
    commEvents.clear();
    waitForGraph.clear();
    taskToRole.clear();
    roleSizes.clear();
    spawnedRoles.clear();
    
    // Extract metadata (roles, tasks, spawns)
    extractMetadata(program);
    
    // Extract all communication events
    extractCommEvents(program);
    
    // Build wait-for graph
    buildWaitForGraph();
    
    // Run deadlock detection algorithms
    detectCircularWait();
    detectSendRecvMismatches();
    detectGatherSpawnInconsistencies();
}

void DeadlockAnalyzer::extractMetadata(Program* program) {
    // Extract role information
    for (const auto& decl : program->decls) {
        if (auto roleDecl = dynamic_cast<RoleDecl*>(decl.get())) {
            roleSizes[roleDecl->id] = roleDecl->size;
        }
    }
    
    // Extract task-to-role mapping and spawned roles
    for (const auto& decl : program->decls) {
        if (auto taskDecl = dynamic_cast<TaskDecl*>(decl.get())) {
            std::string roleName = taskDecl->target->id;
            taskToRole[taskDecl->id] = roleName;
            
            // Check for spawns within this task
            std::vector<Stmt*> stmts;
            for (auto& s : taskDecl->stmts) {
                stmts.push_back(s.get());
            }
            
            // BFS to find all spawns (including nested in if/else)
            size_t idx = 0;
            while (idx < stmts.size()) {
                Stmt* stmt = stmts[idx++];
                if (auto spawn = dynamic_cast<SpawnStmt*>(stmt)) {
                    for (auto& target : spawn->on_targets) {
                        spawnedRoles.insert(target->id);
                    }
                }
                else if (auto ifStmt = dynamic_cast<IfStmt*>(stmt)) {
                    for (auto& s : ifStmt->if_body) stmts.push_back(s.get());
                    for (auto& s : ifStmt->else_body) stmts.push_back(s.get());
                }
            }
        }
    }
}

void DeadlockAnalyzer::extractCommEvents(Program* program) {
    for (const auto& decl : program->decls) {
        if (auto taskDecl = dynamic_cast<TaskDecl*>(decl.get())) {
            std::string roleName = taskDecl->target->id;
            extractTaskCommEvents(taskDecl, roleName);
        }
    }
}

void DeadlockAnalyzer::extractTaskCommEvents(TaskDecl* task, const std::string& roleName) {
    for (const auto& stmt : task->stmts) {
        extractStmtCommEvents(stmt.get(), roleName, task->id);
    }
}

void DeadlockAnalyzer::extractStmtCommEvents(Stmt* stmt, const std::string& currentRole, const std::string& taskName) {
    if (auto send = dynamic_cast<SendStmt*>(stmt)) {
        std::string targetRole = send->target->id;
        commEvents.emplace_back(CommEvent::SEND, currentRole, targetRole, taskName);
    }
    else if (auto recv = dynamic_cast<RecvStmt*>(stmt)) {
        std::string fromRole = recv->from->id;
        commEvents.emplace_back(CommEvent::RECV, currentRole, fromRole, taskName);
    }
    else if (auto broadcast = dynamic_cast<BroadcastStmt*>(stmt)) {
        for (const auto& target : broadcast->targets) {
            commEvents.emplace_back(CommEvent::BROADCAST, currentRole, target->id, taskName);
        }
    }
    else if (auto gather = dynamic_cast<GatherStmt*>(stmt)) {
        for (const auto& target : gather->from_targets) {
            commEvents.emplace_back(CommEvent::GATHER, currentRole, target->id, taskName);
        }
    }
    else if (auto spawn = dynamic_cast<SpawnStmt*>(stmt)) {
        for (const auto& target : spawn->on_targets) {
            commEvents.emplace_back(CommEvent::SPAWN, currentRole, target->id, taskName);
        }
    }
    else if (auto ifStmt = dynamic_cast<IfStmt*>(stmt)) {
        // Recursively analyze both branches
        for (const auto& s : ifStmt->if_body) {
            extractStmtCommEvents(s.get(), currentRole, taskName);
        }
        for (const auto& s : ifStmt->else_body) {
            extractStmtCommEvents(s.get(), currentRole, taskName);
        }
    }
}

void DeadlockAnalyzer::buildWaitForGraph() {
    // Initialize nodes
    for (const auto& pair : roleSizes) {
        waitForGraph[pair.first] = std::set<std::string>();
    }
    
    // Add edges based on blocking operations
    for (const auto& event : commEvents) {
        if (event.type == CommEvent::RECV) {
            // recv creates a wait dependency
            waitForGraph[event.from_role].insert(event.to_role);
        }
        else if (event.type == CommEvent::GATHER) {
            // gather creates a wait dependency
            waitForGraph[event.from_role].insert(event.to_role);
        }
    }
}

void DeadlockAnalyzer::detectCircularWait() {
    std::set<std::string> visited;
    std::set<std::string> recStack;
    std::vector<std::string> cycle;
    
    for (const auto& pair : waitForGraph) {
        if (visited.find(pair.first) == visited.end()) {
            if (hasCycle(pair.first, visited, recStack, cycle)) {
                // Found a cycle
                std::string desc = "Circular wait detected: ";
                for (size_t i = 0; i < cycle.size(); ++i) {
                    desc += cycle[i];
                    if (i < cycle.size() - 1) desc += " → ";
                }
                
                // Find tasks involved
                std::vector<std::string> tasks;
                for (const auto& role : cycle) {
                    for (const auto& pair : taskToRole) {
                        if (pair.second == role) {
                            tasks.push_back(pair.first);
                        }
                    }
                }
                
                addWarning("circular_wait", desc, cycle, tasks);
                return;  // Report first cycle found
            }
        }
    }
}

bool DeadlockAnalyzer::hasCycle(const std::string& node, std::set<std::string>& visited, std::set<std::string>& recStack, std::vector<std::string>& cycle) {
    visited.insert(node);
    recStack.insert(node);
    
    for (const auto& neighbor : waitForGraph[node]) {
        if (recStack.find(neighbor) != recStack.end()) {
            // Found a cycle, now reconstruct it
            cycle.clear();
            cycle.push_back(neighbor);
            cycle.push_back(node);
            return true;
        }
        
        if (visited.find(neighbor) == visited.end()) {
            if (hasCycle(neighbor, visited, recStack, cycle)) {
                if (!cycle.empty() && cycle[0] != node) {
                    cycle.push_back(node);
                }
                return true;
            }
        }
    }
    
    recStack.erase(node);
    return false;
}

void DeadlockAnalyzer::detectSendRecvMismatches() {
    // Build map of sends, (from_role, to_role) -> count
    std::map<std::pair<std::string, std::string>, int> sends;
    std::map<std::pair<std::string, std::string>, int> recvs;
    
    for (const auto& event : commEvents) {
        if (event.type == CommEvent::SEND || event.type == CommEvent::BROADCAST) {
            sends[{event.from_role, event.to_role}]++;
        }
        else if (event.type == CommEvent::RECV) {
            // recv from X means current_role receives from to_role
            recvs[{event.from_role, event.to_role}]++;
        }
    }
    
    // Check for unmatched sends
    for (const auto& send : sends) {
        std::string from = send.first.first;
        std::string to = send.first.second;
        
        // Look for corresponding recv in 'to' from 'from'
        if (recvs.find({to, from}) == recvs.end()) {
            std::string desc = "Unmatched send: " + from + " sends to " + to + ", but " + to + " has no corresponding recv from " + from;
            addWarning("unmatched_send", desc, {from, to}, {});
        }
    }
    
    // Check for potential send-send deadlock
    for (const auto& send1 : sends) {
        std::string role_a = send1.first.first;
        std::string role_b = send1.first.second;
        
        // Check if B also sends to A
        if (sends.find({role_b, role_a}) != sends.end()) {
            // Both A sends to B and B sends to A
            // Check if both have recvs after sends (would deadlock)
            bool a_recvs_from_b = recvs.find({role_a, role_b}) != recvs.end();
            bool b_recvs_from_a = recvs.find({role_b, role_a}) != recvs.end();
            
            if (a_recvs_from_b && b_recvs_from_a) {
                std::string desc = "Potential send-send deadlock: " + role_a + " and " + role_b + " both send to each other before receiving";
                addWarning("send_send_deadlock", desc, {role_a, role_b}, {});
            }
        }
    }
}

void DeadlockAnalyzer::detectGatherSpawnInconsistencies() {
    // Check if any gather waits for roles that were never spawned
    for (const auto& event : commEvents) {
        if (event.type == CommEvent::GATHER) {
            std::string target = event.to_role;
            if (spawnedRoles.find(target) == spawnedRoles.end()) {
                std::string desc = "Gather waiting for role '" + target + 
                                 "' which is never spawned (potential deadlock)";
                addWarning("gather_without_spawn", desc, {event.from_role, target}, {event.task_name});
            }
        }
    }
}

void DeadlockAnalyzer::addWarning(const std::string& type, const std::string& desc, const std::vector<std::string>& roles, const std::vector<std::string>& tasks) {
    DeadlockWarning w;
    w.type = type;
    w.description = desc;
    w.involved_roles = roles;
    w.involved_tasks = tasks;
    warnings.push_back(w);
}

void DeadlockAnalyzer::printWarnings() const {
    for (const auto& warning : warnings) {
        std::cerr << "  [DEADLOCK WARNING] " << warning.description << std::endl;
        
        if (!warning.involved_roles.empty()) {
            std::cerr << "    Roles involved: ";
            for (size_t i = 0; i < warning.involved_roles.size(); ++i) {
                std::cerr << warning.involved_roles[i];
                if (i < warning.involved_roles.size() - 1) std::cerr << ", ";
            }
            std::cerr << std::endl;
        }
        
        if (!warning.involved_tasks.empty()) {
            std::cerr << "    Tasks involved: ";
            for (size_t i = 0; i < warning.involved_tasks.size(); ++i) {
                std::cerr << warning.involved_tasks[i];
                if (i < warning.involved_tasks.size() - 1) std::cerr << ", ";
            }
            std::cerr << std::endl;
        }
        std::cerr << std::endl;
    }
}
