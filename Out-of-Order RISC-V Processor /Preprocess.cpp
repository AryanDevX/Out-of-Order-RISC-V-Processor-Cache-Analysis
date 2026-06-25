#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>

static std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: ./preprocess <filename.s>\n";
        return 1;
    }

    std::ifstream fin(argv[1]);
    if (!fin) {
        std::cerr << "Cannot open file: " << argv[1] << "\n";
        return 1;
    }

    std::unordered_map<std::string, int> mem_labels; 
    std::unordered_map<std::string, int> code_labels;
    std::vector<std::string> raw_inst;
    std::vector<std::string> mem_lines;

    int mem_address = 0;

    std::string line;
    while (std::getline(fin, line)) {
        size_t hash = line.find('#');
        if (hash != std::string::npos) line = line.substr(0, hash);
        line = trim(line);
        if (line.empty()) continue;

        if (line[0] == '.') {
            size_t colon = line.find(':');
            if (colon == std::string::npos) continue;
            std::string label = trim(line.substr(1, colon - 1));
            mem_labels[label] = mem_address;
            std::istringstream ss(line.substr(colon + 1));
            std::string tok;
            while (ss >> tok) mem_address++;
            mem_lines.push_back(line);
        } else {
            raw_inst.push_back(line);
        }
    }
    fin.close();

    std::vector<std::string> unlabelled;
    for (auto& instr : raw_inst) {
        size_t colon = instr.find(':');
        if (colon != std::string::npos) {
            std::string label = trim(instr.substr(0, colon));
            code_labels[label] = (int)unlabelled.size();
            std::string rest = trim(instr.substr(colon + 1));
            if (!rest.empty()) unlabelled.push_back(rest);
        } else {
            unlabelled.push_back(instr);
        }
    }

    auto resolve_branch_target = [&](const std::string& target, int pc) -> std::string {
        auto it = code_labels.find(target);
        if (it != code_labels.end())
            return std::to_string(it->second - pc);
        return target; 
    };

    auto resolve_mem_operand = [&](const std::string& operand) -> std::string {
        size_t lp = operand.find('(');
        size_t rp = operand.find(')');
        if (lp == std::string::npos || rp == std::string::npos) return operand;
        std::string offset = operand.substr(0, lp);
        std::string reg    = operand.substr(lp + 1, rp - lp - 1);
        auto it = mem_labels.find(offset);
        if (it != mem_labels.end())
            return std::to_string(it->second) + "(" + reg + ")";
        return operand; 
    };

    const std::vector<std::string> branch_ops = {"beq", "bne", "blt", "ble"};
    const std::vector<std::string> jump_ops   = {"j"};
    const std::vector<std::string> mem_ops    = {"lw", "sw"};

    auto is_one_of = [](const std::string& s, const std::vector<std::string>& v) {
        for (auto& x : v) if (x == s) return true;
        return false;
    };

    auto tokenise = [](const std::string& line) -> std::vector<std::string> {
        std::vector<std::string> tokens;
        std::string cur;
        for (char c : line) {
            if (c == ',' || c == ' ' || c == '\t') {
                if (!cur.empty()) { tokens.push_back(cur); cur.clear(); }
            } else {
                cur += c;
            }
        }
        if (!cur.empty()) tokens.push_back(cur);
        return tokens;
    };

    std::ofstream fout(argv[1]); 
    if (!fout) {
        std::cerr << "Cannot write to file: " << argv[1] << "\n";
        return 1;
    }

    for (const auto& mline : mem_lines) {
        fout << mline << "\n";
    }

    for (int pc = 0; pc < (int)unlabelled.size(); pc++) {
        auto tokens = tokenise(unlabelled[pc]);
        if (tokens.empty()) continue;
        std::string mnemonic = tokens[0];

        if (is_one_of(mnemonic, jump_ops) && tokens.size() >= 2) {
            tokens[1] = resolve_branch_target(tokens[1], pc);

        } else if (is_one_of(mnemonic, branch_ops) && tokens.size() >= 4) {
            tokens[3] = resolve_branch_target(tokens[3], pc);

        } else if (is_one_of(mnemonic, mem_ops) && tokens.size() >= 3) {
            tokens[2] = resolve_mem_operand(tokens[2]);
        }

        fout << tokens[0];
        for (int i = 1; i < (int)tokens.size(); i++)
            fout << (i == 1 ? " " : ", ") << tokens[i];
        fout << "\n";
    }

    std::cout << "[preprocess] done: " << unlabelled.size()
              << " instructions written to " << argv[1] << "\n";
    return 0;
}