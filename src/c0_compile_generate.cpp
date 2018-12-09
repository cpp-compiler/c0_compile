#include <string>
#include <iostream>
#include "c0_compile_utils.hpp"
#include "c0_compile_symbol.hpp"
#include "c0_compile_lexical_analysis.hpp"
#include "c0_compile_gramma.hpp"
#include "c0_compile_pcode.hpp"

using std::string;
using std::cout;
using std::endl;

extern SymbolTableTree* symbol_table_tree;
extern SymbolQueue* handle_symbol_queue;
extern SymbolQueue* handle_correct_queue;
extern PcodeGenerator* pcode_generator;

inline bool IsAddOperation(SymbolName name) {
    return (name == ADD_SYM) || (name == SUB_SYM);
}

inline bool IsMultiplyOperation(SymbolName name) {
    return (name == MUL_SYM) || (name == DIV_SYM);
}

inline bool IsValidVariableType(SymbolName name) {
    return (name == INT_SYM) || (name == CHAR_SYM);
}

inline bool IsValidFunctionType(SymbolName name) {
    return (name == INT_SYM) || (name == CHAR_SYM) || (name == VOID_SYM);
}

inline bool IsRelationalOpeartor(SymbolName name) {
    return (name == EQUAL_SYM ||
            name == NOT_EQUAL_SYM ||
            name == SMALL_SYM ||
            name == SMALL_EQUAL_SYM ||
            name == LARGE_SYM ||
            name == LARGE_EQUAL_SYM);
}

inline int SymbolTypeMap(SymbolType& item) {
    switch (item) {
        case CHAR: return 1;
        case INT: return 2;
        default: fprintf(stderr, "inlvaid symbol type");
    }
    return -2;
}

inline SymbolType SymbolTypeInverseMap(int map_value) {
    switch (map_value) {
        case 1: return CHAR;
        case 2: return INT;
        default: fprintf(stderr, "inlvaid symbol type");
    }
    return VOID;
}

inline SymbolType MaxDataType(SymbolType& item1, SymbolType& item2) {
    int value1 = SymbolTypeMap(item1);
    int value2 = SymbolTypeMap(item2);
    int max_value = value1 >= value2 ? value1 : value2;
    return SymbolTypeInverseMap(max_value);
}

inline SymbolType Name2Type(SymbolName name) {
    switch (name) {
        case (INT_SYM): return INT;
        case (CHAR_SYM): return CHAR;
        case (VOID_SYM): return VOID;
    }
}


compile_errcode Factor::Generate(string& factor_string) {
    int ret = COMPILE_OK;
    string expression_string;
    SymbolName name = handle_correct_queue->GetCurrentName();
    string temp;
    switch (name) {
        case INTERGER_SYM: {
            int value = handle_correct_queue->GetCurrentValue<int>();
            factor_string = std::to_string(value);
            break;
        }
        case CHARACTER_SYM: {
            int value = static_cast<int>(handle_correct_queue->GetCurrentValue<char>());
            factor_string = std::to_string(value);
            break;
        }
        case IDENTIFIER_SYM: {
            m_identifier_name = handle_correct_queue->GetCurrentValue<string>();
            handle_correct_queue->SetCacheLocate();
            handle_correct_queue->NextSymbol();
            name = handle_correct_queue->GetCurrentName();
            if (name == L_SQUARE_BRACKET_SYM) {
                string array_addr;
                symbol_table_tree->GetAddressString(m_identifier_name, array_addr);
                temp = pcode_generator->TempNameGenerator();
                handle_correct_queue->NextSymbol();
                m_expression.Generate(expression_string);
                Pcode pcode(ADD, temp, array_addr, expression_string);
                pcode_generator->Insert(pcode);
                factor_string = temp;
            } else if (name == L_CIRCLE_BRACKET_SYM) {
                handle_correct_queue->NextSymbol();
                m_value_argument_list.Generate();
                //TODO Pcode pcode(JUMP);
            } else {
                handle_correct_queue->SetCurrentLocate();
                SymbolKind kind; 
                SymbolType type;
                symbol_table_tree->GetTermKind(m_identifier_name, kind);
                symbol_table_tree->GetTermType(m_identifier_name, type);
                if (kind == CONST) {
                    if (type == INT) {
                        int value = handle_correct_queue->GetCurrentValue<int>();
                        factor_string = std::to_string(value);
                    } else if (type == CHAR) {
                        int value = static_cast<int>(handle_correct_queue->GetCurrentValue<char>());
                        factor_string = std::to_string(value);
                    }
                } else {
                    symbol_table_tree->GetAddressString(m_identifier_name, factor_string);
                }
                break;
            }
        }
        case L_CIRCLE_BRACKET_SYM: {
            handle_correct_queue->NextSymbol();
            m_expression.Generate(factor_string);
            break;
        }
    }
    handle_correct_queue->NextSymbol();
    return COMPILE_OK;
}

compile_errcode Term::Generate(string& term_string) {
    m_factor = new Factor;
    int ret = COMPILE_OK;
    int state = 0;
    string multi_identity("1");
    string factor_string1 = multi_identity;
    string factor_string2;
    string temp;
    PcodeType pcode_type = MUL;
    while (true) {
        SymbolName name = handle_correct_queue->GetCurrentName();
        switch (state) {
            case 0: {
                temp = pcode_generator->TempNameGenerator();
                m_factor->Generate(factor_string2);
                Pcode pcode(pcode_type, temp, factor_string1, factor_string2);
                pcode_generator->Insert(pcode);
                factor_string1 = temp;
                state = 1;
                break;
            }
            case 1: {
                if (IsMultiplyOperation(name)) {
                    if (name == MUL_SYM) {
                        pcode_type = MUL;
                    } else {
                        pcode_type = DIV;
                    }
                    state = 0;
                    break;
                } else {
                    delete(m_factor);
                    return COMPILE_OK;
                }
            }
        }
        if (state == 0)
            handle_correct_queue->NextSymbol();
    }
}

compile_errcode Expression::Generate(string& expression_string) {
    int ret = COMPILE_OK;
    int state = 0;
    string add_identity("0");
    string term_string1 = add_identity;
    string term_string2;
    string temp;
    PcodeType pcode_type = ADD;
    while (true) {
        SymbolName name = handle_correct_queue->GetCurrentName();
        switch (state) {
            case 0: {
                if (IsAddOperation(name)) {
                    if (name == SUB_SYM) {
                        pcode_type = SUB;
                    }
                    state = 0;
                    break;
                } else {
                    m_term.Generate(term_string2);
                    temp = pcode_generator->TempNameGenerator();
                    Pcode pcode(pcode_type, temp, term_string1, term_string2);
                    pcode_generator->Insert(pcode);
                    term_string1 = temp;
                    state = 1;
                    break;
                }
            }
            case 1: {
                if (IsAddOperation(name)) {
                    if (name == ADD_SYM) {
                        pcode_type = ADD;
                    } else {
                        pcode_type = SUB;
                    }
                    state = 2;
                    break;
                } else {
                    return COMPILE_OK;
                }
            }
            case 2: {
                m_term.Generate(term_string2);
                temp = pcode_generator->TempNameGenerator();
                Pcode pcode(pcode_type, temp, term_string1, term_string2);
                pcode_generator->Insert(pcode);
                term_string1 = temp;
                state = 1;
            }
        }
        if (state != 1) {
            handle_correct_queue->NextSymbol();
        }
    }
}

compile_errcode ConstantDefinition::Generate() {
    int state = 0;
    int ret = 0;
    while (true) {
        SymbolName name = handle_correct_queue->GetCurrentName();
        int line_number = handle_correct_queue->GetCurrentLine();
        int character_number = handle_correct_queue->GetCurrentCharacter();
        switch (state) {
            case 0: {
                if (name == CONST_SYM) {
                    state = 1;
                    break;
                } else {
                    return NOT_MATCH;
                }
            }
            case 1: {
                if (IsValidVariableType(name)) {
                    state = 2;
                    m_type = name;
                    break;
                } else {
                    return INVALID_TYPE_ERROR;
                }
            }
            case 2: {
                if (name == IDENTIFIER_SYM) {
                    state = 3;
                    m_identifier_name = handle_correct_queue->GetCurrentValue<string>();
                    if (symbol_table_tree->Find(m_identifier_name, true)) {
                        m_valid = false;
                    } else {
                        m_valid = true;
                    }
                    break;
                } else {
                    return NOT_MATCH;
                }
            }
            case 3: {
                if (name == ASSIGN_SYM) {
                    state = 4;
                } else {
                    return NOT_MATCH;
                }
                break;
            }
            case 4: {
                if (name == INTERGER_SYM || name == CHARACTER_SYM) {
                    state = 5;
                    if (m_type == INT_SYM && name != INTERGER_SYM) {
                        break;
                    }
                    if (m_type == CHAR_SYM && name != CHARACTER_SYM) {
                        break;
                    }
                    if (m_valid) {
                        SymbolTableTerm term(m_identifier_name, CONST, m_type);
                        if (name == INTERGER_SYM)
                            term.SetConstInformation<int>(handle_correct_queue->GetCurrentValue<int>());
                        else if (name == CHARACTER_SYM)
                            term.SetConstInformation<char>(handle_correct_queue->GetCurrentValue<char>());
                        symbol_table_tree->Insert(term);
                    }
                    break;
                } else {
                    return NOT_MATCH;
                }
            }
            case 5: {
                if (name == COMMA_SYM) {
                    state = 2;
                } else if (name == SEMICOLON_SYM) {
                    state = 6;
                } else {
                   return NOT_MATCH;
                }
                break;
            }
            case 6: return COMPILE_OK;
        }
        handle_correct_queue->NextSymbol();
    }
}

compile_errcode ConstantDeclaration::Generate() {
    int ret = 0;
    int correct_count = 0;
    while (true) {
        SymbolName name = handle_correct_queue->GetCurrentName();
        if (name == CONST_SYM) {
            if ((ret = m_constant_definition.Generate()) == COMPILE_OK) {
                m_constant_definition.LogOutput();
                correct_count++;
            } else {
                return NOT_MATCH;
            }
        } else {
            if (correct_count > 0)
                return COMPILE_OK;
            else
                return NOT_MATCH;
        }
    }
}

compile_errcode VariableDefinition::Generate() {
    int state = 0;
    while (true) {
        SymbolName name = handle_correct_queue->GetCurrentName();
        int line_number = handle_correct_queue->GetCurrentLine();
        int character_number = handle_correct_queue->GetCurrentCharacter();
        switch (state) {
            case 0: {
                if (IsValidVariableType(name)) {
                    state = 1;
                    m_type = name;
                    break;
                } else {
                    return NOT_MATCH;
                }
            }
            case 1: {
                if (name == IDENTIFIER_SYM) {
                    state = 2;
                    m_identifier_name = handle_correct_queue->GetCurrentValue<string>();
                    if (symbol_table_tree->Find(m_identifier_name, true)) {
                        m_valid = false;
                    } else {
                        m_valid = true;
                    }
                    break;
                } else {
                    return NOT_MATCH;
                }
            }
            case 2: {
                if (name == L_SQUARE_BRACKET_SYM) {
                    state = 11;
                    break;
                } else if (name == COMMA_SYM) {
                    state = 1;
                    if (m_valid) {
                        SymbolTableTerm term(m_identifier_name, VARIABLE, m_type);
                        symbol_table_tree->Insert(term);
                    }
                    break;
                } else if (name == SEMICOLON_SYM) {
                    state = 3;
                    if (m_valid) {
                        SymbolTableTerm term(m_identifier_name, VARIABLE, m_type);
                        symbol_table_tree->Insert(term);
                    }
                    break;
                } else {
                    return NOT_MATCH;
                }
            }
            case 3: return COMPILE_OK;
            case 11: {
                if (name == INTERGER_SYM) {
                    state = 12;
                    m_array_length = handle_correct_queue->GetCurrentValue<int>();
                    if (m_array_length <= 0) {
                        m_valid = false;
                    }
                    break;
                } else {
                    return NOT_MATCH;
                }
            }
            case 12: {
                if (name == R_SQUARE_BRACKET_SYM) {
                    state = 13;
                    if (m_valid) {
                        SymbolTableTerm term(m_identifier_name, ARRAY, m_type);
                        term.SetArrayInformation(m_array_length);
                        symbol_table_tree->Insert(term);
                    }
                    break;
                } else {
                    return NOT_MATCH;
                }
            }
            case 13: {
                if (name == COMMA_SYM) {
                    state = 1;
                    break;
                } else if (name == SEMICOLON_SYM) {
                    state = 3;
                    break;
                } else {
                    return NOT_MATCH;
                }
            }
        }
        handle_correct_queue->NextSymbol();
    }
}

compile_errcode VariableDeclaration::Generate() {
    int ret = 0;
    int correct_count = 0;
    while (true) {
        handle_correct_queue->SetCacheLocate();
        SymbolName name = handle_correct_queue->GetCurrentName();
        if (IsValidVariableType(name)) {
            handle_correct_queue->NextSymbol();
            name = handle_correct_queue->GetCurrentName();
            if (name == IDENTIFIER_SYM) {
                handle_correct_queue->NextSymbol();
                name = handle_correct_queue->GetCurrentName();
                if (name == L_CIRCLE_BRACKET_SYM) {
                    handle_correct_queue->SetCurrentLocate();
                    break;
                } else {
                    handle_correct_queue->SetCurrentLocate();
                }
            } else {
                break;
            }
        } else {
            if (correct_count > 0)
                return COMPILE_OK;
            else
                return NOT_MATCH;
        }
        if ((ret = m_variable_definition.Generate()) == COMPILE_OK) {
            m_variable_definition.LogOutput();
            correct_count++;
        } else {
            return NOT_MATCH;
        }
    }
    return NOT_MATCH;
}

compile_errcode InputStatement::Generate() {
    int ret = COMPILE_OK;
    int state = 0;
    while (true) {
        SymbolName name = handle_correct_queue->GetCurrentName();
        switch (state) {
            case 0: {
                if (name == SCANF_SYM) {
                    state = 1;
                    break;
                } else {
                    return NOT_MATCH;
                }
            }
            case 1: {
                if (name == L_CIRCLE_BRACKET_SYM) {
                    state = 2;
                    break;
                } else {
                    return NOT_MATCH;
                }
            }
            case 2: {
                if (name == IDENTIFIER_SYM) {
                    state = 3;
                    break;
                } else {
                    return NOT_MATCH;
                }
            }
            case 3: {
                if (name == COMMA_SYM) {
                    state = 2;
                    break;
                } else if (name == R_CIRCLE_BRACKET_SYM) {
                    state = 4;
                    break;
                } else {
                    return NOT_MATCH;
                }
            }
            case 4: return COMPILE_OK;
        }
        handle_correct_queue->NextSymbol();
    }
}

compile_errcode ReturnStatement::Generate(SymbolType& function_type, string funtion_name) {
    int ret = COMPILE_OK;
    int state = 0;
    int line_number = handle_correct_queue->GetCurrentLine();
    int character_number = handle_correct_queue->GetCurrentCharacter();
    SymbolType expression_type;
    while (true) {
        SymbolName name = handle_correct_queue->GetCurrentName();
        switch (state) {
            case 0: {
                if (name == RETURN_SYM) {
                    state = 1;
                    break;
                } else {
                    return NOT_MATCH;
                }
            }
            case 1: {
                if (name == L_CIRCLE_BRACKET_SYM) {
                    state = 2;
                    break;
                } else if (name == SEMICOLON_SYM) {
                    if (function_type != VOID) {
                    }
                    return COMPILE_OK;
                } else {
                    return NOT_MATCH;
                }
            }
            case 2: {
                if ((ret = m_expression.Generate()) == COMPILE_OK) {
                    state = 3;
                    break;
                } else {
                    return NOT_MATCH;
                }
            }
            case 3: {
                if (name == R_CIRCLE_BRACKET_SYM) {
                    state = 4;
                    break;
                } else {
                    return NOT_MATCH;
                }
            }
            case 4: return COMPILE_OK;
        }
        if (state != 3)
            handle_correct_queue->NextSymbol();
    }
}

compile_errcode OutputStatement::Generate() {
    int ret = COMPILE_OK;
    int state = 0;
    SymbolType expression_type;
    while (true) {
        SymbolName name = handle_correct_queue->GetCurrentName();
        switch (state) {
            case 0: {
                if (name == PRINTF_SYM) {
                    state = 1;
                    break;
                } else {
                    return NOT_MATCH;
                }
            }
            case 1: {
                if (name == L_CIRCLE_BRACKET_SYM) {
                    state = 2;
                    break;
                } else {
                    return NOT_MATCH;
                }
            }
            case 2: {
                if (name == STRING_SYM) {
                    string str("This is a string");
                    GRAMMA_LOG(str);
                    state = 3;
                    break;
                } else if ((ret = m_expression.Generate()) == COMPILE_OK) {
                    state = 5;
                    break;
                } else {
                    return NOT_MATCH;
                }
            }
            case 3: {
                if (name == COMMA_SYM) {
                    state = 4;
                    break;
                } else if (name == R_CIRCLE_BRACKET_SYM) {
                    state = 6;
                    break;
                } else {
                    return NOT_MATCH;
                }
            }
            case 4: {
                if ((ret = m_expression.Generate()) == COMPILE_OK) {
                    state = 5;
                    break;
                } else {
                    return NOT_MATCH;
                }
            }
            case 5: {
                if (name == R_CIRCLE_BRACKET_SYM) {
                    state = 6;
                    break;
                } else {
                    return NOT_MATCH;
                }
            }
            case 6: return COMPILE_OK;
        }
        if (state != 5)
            handle_correct_queue->NextSymbol();
    }
}

compile_errcode AssignStatement::Generate() {
    int ret = COMPILE_OK;
    int state = 0;
    SymbolType expression_type;
    SymbolType identifier_type;
    SymbolKind identifier_kind;
    while (true) {
        SymbolName name = handle_correct_queue->GetCurrentName();
        switch (state) {
            case 0: {
                if (name == IDENTIFIER_SYM) {
                    state = 1;
                    break;
                } else {
                    return NOT_MATCH;
                }
            }
            case 1: {
                if (name == ASSIGN_SYM) {
                    state = 2;
                    break;
                } else if (name == L_SQUARE_BRACKET_SYM) {
                    state = 11;
                    break;
                } else {
                    return NOT_MATCH;
                }
            }
            case 2: {
                if ((ret = m_expression.Generate()) == COMPILE_OK) {
                    state = 3;
                    break;
                } else {
                    return NOT_MATCH;
                }
            }
            case 3: return COMPILE_OK;
            case 11: {
                if ((ret = m_expression.Generate()) == COMPILE_OK) {
                    m_expression.LogOutput();
                    state = 12;
                    break;
                } else {
                    return NOT_MATCH;
                }
            }
            case 12: {
                if (name == R_SQUARE_BRACKET_SYM) {
                    state = 13;
                    break;
                } else {
                    return NOT_MATCH;
                }
            }
            case 13: {
                if (name == ASSIGN_SYM) {
                    state = 2;
                    break;
                } else {
                    return NOT_MATCH;
                }
            }
        }
        if (state != 3 && state != 12)
            handle_correct_queue->NextSymbol();
    }
}

compile_errcode Condition::Generate() {
    int ret = COMPILE_OK;
    int state = 0;
    while (true) {
        SymbolName name = handle_correct_queue->GetCurrentName();
        switch (state) {
            case 0: {
                if ((ret = m_expression.Generate()) == COMPILE_OK) {
                    m_expression.LogOutput();
                    state = 1;
                    break;
                } else {
                    return NOT_MATCH;
                }
            }
            case 1: {
                if (IsRelationalOpeartor(name)) {
                    state = 2;
                    break;
                } else {
                    return COMPILE_OK;
                }
            }
            case 2: {
                if ((ret = m_expression.Generate()) == COMPILE_OK) {
                    m_expression.LogOutput();
                    state = 3;
                    break;
                } else {
                    return NOT_MATCH;
                }
            }
            case 3: return COMPILE_OK;
        }
        if (state == 2)
            handle_correct_queue->NextSymbol();
    }
}

compile_errcode ConditionStatement::Generate() {
    int ret = COMPILE_OK;
    int state = 0;
    m_statement_ptr = new Statement;
    while (true) {
        SymbolName name = handle_correct_queue->GetCurrentName();
        switch (state) {
            case 0: {
                if (name == IF_SYM) {
                    state = 1;
                    break;
                } else {
                    goto ERROR_IF;
                }
            }
            case 1: {
                if (name == L_CIRCLE_BRACKET_SYM) {
                    state = 2;
                    break;
                } else {
                    goto ERROR_IF;
                }
            }
            case 2: {
                if ((ret = m_condition.Generate()) == COMPILE_OK) {
                    m_condition.LogOutput();
                    state = 3;
                    break;
                } else {
                    goto ERROR_IF;
                }
            }
            case 3: {
                if (name == R_CIRCLE_BRACKET_SYM) {
                    state = 4;
                    break;
                } else {
                    goto ERROR_IF;
                }
            }
            case 4: {
                if ((ret = m_statement_ptr->Generate()) == COMPILE_OK) {
                    m_statement_ptr->LogOutput();
                    state = 5;
                    break;
                } else {
                    goto ERROR_IF;
                }
            }
            case 5: goto CORRECT_IF;
        }
        if (state != 3 && state != 5)
            handle_correct_queue->NextSymbol();
    }
    CORRECT_IF:
        delete(m_statement_ptr);
        return COMPILE_OK;
    ERROR_IF:
        delete(m_statement_ptr);
        return NOT_MATCH;
}

compile_errcode WhileLoopStatement::Generate() {
    int ret = COMPILE_OK;
    int state = 0;
    m_statement_ptr = new Statement;
    while (true) {
        SymbolName name = handle_correct_queue->GetCurrentName();
        switch (state) {
            case 0: {
                if (name == WHILE_SYM) {
                    state = 1;
                    break;
                } else {
                    goto ERROR_WHILE;
                }
            }
            case 1: {
                if (name == L_CIRCLE_BRACKET_SYM) {
                    state = 2;
                    break;
                } else {
                    goto ERROR_WHILE;
                }
            }
            case 2: {
                if ((ret = m_condition.Generate()) == COMPILE_OK) {
                    m_condition.LogOutput();
                    state = 3;
                    break;
                } else {
                    goto ERROR_WHILE;
                }
            }
            case 3: {
                if (name == R_CIRCLE_BRACKET_SYM) {
                    state = 4;
                    break;
                } else {
                    goto ERROR_WHILE;
                }
            }
            case 4: {
                if ((ret = m_statement_ptr->Generate()) == COMPILE_OK) {
                    m_statement_ptr->LogOutput();
                    state = 5;
                    break;
                } else {
                    goto ERROR_WHILE;
                }
            }
            case 5: goto CORRECT_WHILE;
        }
        if (state != 3 && state != 5)
            handle_correct_queue->NextSymbol();
    }
    CORRECT_WHILE:
        delete(m_statement_ptr);
        return COMPILE_OK;
    ERROR_WHILE:
        delete(m_statement_ptr);
        return NOT_MATCH;
}

compile_errcode SwitchChildStatement::Generate() {
    int ret = COMPILE_OK;
    int state = 0;
    m_statement_ptr = new Statement;
    while (true) {
        SymbolName name = handle_correct_queue->GetCurrentName();
        switch (state) {
            case 0: {
                if (name == CASE_SYM) {
                    state = 1;
                    break;
                } else {
                    goto ERROR_SWITCH_CHILD;
                }
            }
            case 1: {
                if (name == INTERGER_SYM || name == CHARACTER_SYM) {
                    state = 2;
                    break;
                } else {
                    goto ERROR_SWITCH_CHILD;
                }
            }
            case 2: {
                if (name == COLON_SYM) {
                    state = 3;
                    break;
                } else {
                    goto ERROR_SWITCH_CHILD;
                }
            }
            case 3: {
                if ((ret = m_statement_ptr->Generate()) == COMPILE_OK) {
                    m_statement_ptr->LogOutput();
                    state = 4;
                    break;
                } else {
                    goto ERROR_SWITCH_CHILD;
                }
            }
            case 4: goto CORRECT_SWITCH_CHILD;
        }
        if (state != 4)
            handle_correct_queue->NextSymbol();
    }
    CORRECT_SWITCH_CHILD:
        delete(m_statement_ptr);
        return COMPILE_OK;
    ERROR_SWITCH_CHILD:
        delete(m_statement_ptr);
        return NOT_MATCH;
}

compile_errcode SwitchTable::Generate() {
    int ret = 0;
    int count = 0;
    while (true) {
        if ((ret = m_switch_child_statement.Generate()) == COMPILE_OK) {
            m_switch_child_statement.LogOutput();
            count++;
        } else {
            break;
        }
    }
    if (count > 0)
        return COMPILE_OK;
    else
        return NOT_MATCH;
}

compile_errcode Default::Generate() {
    int ret = COMPILE_OK;
    int state = 0;
    while (true) {
        SymbolName name = handle_correct_queue->GetCurrentName();
        switch (state) {
            case 0: {
                if (name == DEFAULT_SYM) {
                    state = 1;
                    break;
                } else {
                    return NOT_MATCH;
                }
            }
            case 1: {
                if (name == COLON_SYM) {
                    state = 2;
                    break;
                } else {
                    return NOT_MATCH;
                }
            }
            case 2: {
                if (name == R_CURLY_BRACKET_SYM) {
                    return COMPILE_OK;
                } else {
                    m_statement_ptr = new Statement;
                    if ((ret = m_statement_ptr->Generate()) == COMPILE_OK) {
                        m_statement_ptr->LogOutput();
                        delete(m_statement_ptr);
                        return COMPILE_OK;
                    } else {
                        delete(m_statement_ptr);
                        return NOT_MATCH;
                    }
                }
            }
        }
        handle_correct_queue->NextSymbol();
    }
}
compile_errcode SwitchStatement::Generate() {
    int ret = COMPILE_OK;
    int state = 0;
    while (true) {
        SymbolName name = handle_correct_queue->GetCurrentName();
        switch (state) {
            case 0: {
                if (name == SWITCH_SYM) {
                    state = 1;
                    break;
                } else {
                    goto ERROR_SWITCH;
                }
            }
            case 1: {
                if (name = L_CIRCLE_BRACKET_SYM) {
                    state = 2;
                    break;
                } else {
                    goto ERROR_SWITCH;
                }
            }
            case 2: {
                if ((ret = m_expression.Generate()) == COMPILE_OK) {
                    m_expression.LogOutput();
                    state = 3;
                    break;
                } else {
                    goto ERROR_SWITCH;
                }
            }
            case 3: {
                if (name == R_CIRCLE_BRACKET_SYM) {
                    state = 4;
                    break;
                } else {
                    goto ERROR_SWITCH;
                }
            }
            case 4: {
                if (name == L_CURLY_BRACKET_SYM) {
                    state = 5;
                    break;
                } else {
                    goto ERROR_SWITCH;
                }
            }
            case 5: {
                if ((ret = m_switch_table.Generate()) == COMPILE_OK) {
                    m_switch_table.LogOutput();
                    state = 6;
                    break;
                } else {
                    goto ERROR_SWITCH;
                }
            }
            case 6: {
                if ((ret = m_default.Generate()) == COMPILE_OK) {
                    m_default.LogOutput();
                    state = 7;
                    break;
                } else {
                    goto ERROR_SWITCH;
                }
            }
            case 7: {
                if (name == R_CURLY_BRACKET_SYM) {
                    state = 8;
                    break;
                } else {
                    goto ERROR_SWITCH;
                }
            }
            case 8: return COMPILE_OK;
        }
        if (state != 3 && state != 6 && state != 7)
            handle_correct_queue->NextSymbol();
    }
    ERROR_SWITCH:
       return NOT_MATCH;
}

compile_errcode Statement::Generate() {
    int ret = COMPILE_OK;
    SymbolType function_type;
    ret = symbol_table_tree->GetCurrentTableType(function_type);
    string funtion_name = symbol_table_tree->GetCurrentTableName();
    SymbolName name = handle_correct_queue->GetCurrentName();
    handle_correct_queue->SetCacheLocate();
    if (name == L_CURLY_BRACKET_SYM) {
        handle_correct_queue->NextSymbol();
        m_statement_list_ptr = new StatementList;
        if ((ret = m_statement_list_ptr->Generate()) == COMPILE_OK) {
            m_statement_list_ptr->LogOutput();
            delete(m_statement_list_ptr);
            name = handle_correct_queue->GetCurrentName();
            if (name == R_CURLY_BRACKET_SYM) {
                handle_correct_queue->NextSymbol();
                goto NO_SEMICOLON;
            } else {
                return NOT_MATCH;
            }
        } else {
            delete(m_statement_list_ptr);
            NOT_MATCH;
        }
    } else if ((ret = m_output_statement.Generate()) == COMPILE_OK) {
        m_output_statement.LogOutput();
        goto SEMICOLON_CHECK;
    } else if (handle_correct_queue->SetCurrentLocate(), (ret = m_input_statement.Generate()) == COMPILE_OK) {
        m_input_statement.LogOutput();
        goto SEMICOLON_CHECK;
    } else if (handle_correct_queue->SetCurrentLocate(), (ret = m_return_statement.Generate(function_type, funtion_name)) == COMPILE_OK) {
        m_return_statement.LogOutput();
        goto SEMICOLON_CHECK;
    } else if (handle_correct_queue->SetCurrentLocate(), (ret = m_assign_statement.Generate()) == COMPILE_OK) {
        m_assign_statement.LogOutput();
        goto SEMICOLON_CHECK;
    } else if (handle_correct_queue->SetCurrentLocate(), (ret = m_function_call.Generate()) == COMPILE_OK) {
        m_function_call.LogOutput();
        goto SEMICOLON_CHECK;
    } else if (handle_correct_queue->SetCurrentLocate(), (ret = m_condition_statement.Generate()) == COMPILE_OK) {
        m_condition_statement.LogOutput();
        goto NO_SEMICOLON;
    } else if (handle_correct_queue->SetCurrentLocate(), (ret = m_while_loop_statement.Generate()) == COMPILE_OK) {
        m_while_loop_statement.LogOutput();
        goto NO_SEMICOLON;
    } else if (handle_correct_queue->SetCurrentLocate(), (ret = m_switch_statement.Generate()) == COMPILE_OK) {
        m_switch_statement.LogOutput();
        goto NO_SEMICOLON;
    } else {
        name = handle_correct_queue->GetCurrentName();
        if (name == SEMICOLON_SYM) {
            handle_correct_queue->NextSymbol();
            string str("This is a empty statement");
            GRAMMA_LOG(str);
            return COMPILE_OK;
        } else {
            return NOT_MATCH;
        }
    }
    SEMICOLON_CHECK:
        name = handle_correct_queue->GetCurrentName();
        if (name == SEMICOLON_SYM) {
            handle_correct_queue->NextSymbol();
            return COMPILE_OK;
        } else {
            fprintf(stderr, "expected a ':' in the end of statement\n");
            return COMPILE_OK;
        }
    NO_SEMICOLON:
        return COMPILE_OK;
}

compile_errcode StatementList::Generate() {
    int ret = COMPILE_OK;
    while (true) {
        handle_correct_queue->SetCacheLocate();
        if ((ret = m_statement.Generate()) == COMPILE_OK) {
            m_statement.LogOutput();
        } else {
            handle_correct_queue->SetCurrentLocate();
            return COMPILE_OK;
        }
    }
}

compile_errcode CompoundStatement::Generate() {
    int ret = COMPILE_OK;
    SymbolName name = handle_correct_queue->GetCurrentName();
    if (name == CONST_SYM) {
        if ((ret = m_constant_declaration.Generate()) == COMPILE_OK) {
            m_constant_declaration.LogOutput();
        }
    }
    if ((ret = m_variable_declaration.Generate()) == COMPILE_OK) {
        m_variable_declaration.LogOutput();
    }
    m_statement_list.Generate();
    return COMPILE_OK;
}

compile_errcode ArgumentList::Generate(int& argument_number) {
    int ret = COMPILE_OK;
    int state = 0;
    argument_number = 0;
    while (true) {
        SymbolName name = handle_correct_queue->GetCurrentName();
        switch (state) {
            case 0: {
                if (name == R_CIRCLE_BRACKET_SYM) {
                    return COMPILE_OK;
                } else if (IsValidVariableType(name)) {
                    state = 1;
                    m_type = name;
                    break;
                } else {
                    return NOT_MATCH;
                }
            }
            case 1: {
                if (name == IDENTIFIER_SYM) {
                    state = 2;
                    argument_number++;
                    m_identifier_name = handle_correct_queue->GetCurrentValue<string>();
                    SymbolTableTerm term(m_identifier_name, PARAMETER, m_type);
                    symbol_table_tree->Insert(term);
                    break;
                } else {
                    return NOT_MATCH;
                }
            }
            case 2: {
                if (name == COMMA_SYM) {
                    state = 3;
                    break;
                } else {
                    return COMPILE_OK;
                }
            }
            case 3: {
                if (IsValidVariableType(name)) {
                    state = 1;
                    m_type = name;
                    break;
                } else {
                    return NOT_MATCH;
                }
            }
        }
        handle_correct_queue->NextSymbol();
    }
}

compile_errcode ValueArgumentList::Generate() {
    int ret = COMPILE_OK;
    int state = 0;
    while (true) {
        SymbolName name = handle_correct_queue->GetCurrentName();
        switch (state) {
            case 0: {
                if (name == R_CIRCLE_BRACKET_SYM) {
                    return COMPILE_OK;
                } else if ((ret = m_expression.Generate()) == COMPILE_OK) {
                    state = 1;
                    break;
                } else {
                    return NOT_MATCH;
                }
            }
            case 1: {
                if (name == COMMA_SYM) {
                    state = 2;
                    break;
                } else {
                    return COMPILE_OK;
                }
            }
            case 2: {
                if ((ret = m_expression.Generate()) == COMPILE_OK) {
                    state = 1;
                    break;
                } else {
                    return NOT_MATCH;
                }
            }
        }
        if (state != 1)
            handle_correct_queue->NextSymbol();
    }
}

compile_errcode FunctionCall::Generate() {
    int ret = COMPILE_OK;
    int state = 0;
    while (true) {
        SymbolName name = handle_correct_queue->GetCurrentName();
        switch (state) {
            case 0: {
                if (name == IDENTIFIER_SYM) {
                    state = 1;
                    break;
                } else {
                    return NOT_MATCH;
                }
            }
            case 1: {
                if (name == L_CIRCLE_BRACKET_SYM) {
                    state = 2;
                    break;
                } else {
                    return NOT_MATCH;
                }
            }
            case 2: {
                if ((ret = m_value_argument_list.Generate()) == COMPILE_OK) {
                    state = 3;
                    break;
                } else {
                    return NOT_MATCH;
                }
            }
            case 3: {
                if (name == R_CIRCLE_BRACKET_SYM) {
                    state = 4;
                    break;
                } else {
                    return NOT_MATCH;
                }
            }
            case 4: return COMPILE_OK;
        }
        if (state != 3)
            handle_correct_queue->NextSymbol();
    }
}

compile_errcode FunctionDefinition::Generate() {
    int ret = COMPILE_OK;
    int state = 0;
    while (true) {
        SymbolName name = handle_correct_queue->GetCurrentName();
        int line_number = handle_correct_queue->GetCurrentLine();
        int character_number = handle_correct_queue->GetCurrentCharacter();
        switch (state) {
            case 0: {
                if (name == VOID_SYM || name == INT_SYM || name == CHAR_SYM) {
                    state = 1;
                    m_type = name;
                    break;
                } else {
                    return NOT_MATCH;
                }
            }
            case 1: {
                if (name == IDENTIFIER_SYM) {
                    m_identifier_name = handle_correct_queue->GetCurrentValue<string>();
                    if (symbol_table_tree->Find(m_identifier_name, false)) {
                        m_valid = false;
                        m_identifier_name = "error_" + m_identifier_name + "_error";
                        string previous_table_name = symbol_table_tree->GetCurrentTableName();
                        symbol_table_tree->CreateTable(m_identifier_name, Name2Type(m_type), previous_table_name);
                        symbol_table_tree->SetCurrentTableName(m_identifier_name);
                    } else {
                        string previous_table_name = symbol_table_tree->GetCurrentTableName();
                        symbol_table_tree->CreateTable(m_identifier_name, Name2Type(m_type), previous_table_name);
                        symbol_table_tree->SetCurrentTableName(m_identifier_name);
                        m_valid = true;
                    }
                    state = 2;
                    break;
                } else {
                    return NOT_MATCH;
                }
            }
            case 2: {
                if (name == L_CIRCLE_BRACKET_SYM) {
                    state = 3;
                    break;
                } else {
                    return NOT_MATCH;
                }
            }
            case 3: {
                if ((ret = m_argument_list.Generate(m_argument_number)) == COMPILE_OK) {
                    m_argument_list.LogOutput();
                    state = 4;
                    break;
                } else {
                    return NOT_MATCH;
                }
            }
            case 4: {
                if (name == R_CIRCLE_BRACKET_SYM) {
                    state = 5;
                    break;
                } else {
                    return NOT_MATCH;
                }
            }
            case 5: {
                if (name == L_CURLY_BRACKET_SYM) {
                    state = 6;
                    break;
                } else {
                    return NOT_MATCH;
                }
            }
            case 6: {
                if ((ret = m_compound_statement.Generate()) == COMPILE_OK) {
                    m_compound_statement.LogOutput();
                    state = 7;
                    break;
                } else {
                    return NOT_MATCH;
                }
            }
            case 7: {
                if (name == R_CURLY_BRACKET_SYM) {
                    symbol_table_tree->UpgradeAddress();
                    string previous_table_name = symbol_table_tree->GetCurrentPreviousTableName();
                    symbol_table_tree->SetCurrentTableName(previous_table_name);
                    SymbolTableTerm term(m_identifier_name, FUNCTION, m_type);
                    term.SetFuncInformation(m_argument_number);
                    symbol_table_tree->Insert(term);
                    state = 8;
                    break;
                } else {
                    return NOT_MATCH;
                }
            }
            case 8: return COMPILE_OK;
        }
        if (state != 4 && state != 7)
            handle_correct_queue->NextSymbol();
    }
}

compile_errcode MainFunction::Generate() {
    int ret = COMPILE_OK;
    int state = 0;
    while (true) {
        SymbolName name = handle_correct_queue->GetCurrentName();
        switch (state) {
            case 0: {
                if (name == VOID_SYM) {
                    state = 1;
                    break;
                } else {
                    return NOT_MATCH;
                }
            }
            case 1: {
                if (name == MAIN_SYM) {
                    state = 2;
                    break;
                } else {
                    return NOT_MATCH;
                }
            }
            case 2: {
                if (name == L_CIRCLE_BRACKET_SYM) {
                    state = 3;
                    break;
                } else {
                    return NOT_MATCH;
                }
            }
            case 3: {
                if (name == R_CIRCLE_BRACKET_SYM) {
                    state = 4;
                    break;
                } else {
                    return NOT_MATCH;
                }
            }
            case 4: {
                if (name == L_CURLY_BRACKET_SYM) {
                    state = 5;
                    break;
                } else {
                    return NOT_MATCH;
                }
            }
            case 5: {
                if ((ret = m_compound_statement.Generate()) == COMPILE_OK) {
                    m_compound_statement.LogOutput();
                    state = 6;
                    break;
                } else {
                    return NOT_MATCH;
                }
            }
            case 6: {
                if (name == R_CURLY_BRACKET_SYM) {
                    state = 7;
                    break;
                } else {
                    return NOT_MATCH;
                }
            }
            case 7: return COMPILE_OK;
        }
        if (state != 6)
            handle_correct_queue->NextSymbol();
    }
}

compile_errcode Program::Generate() {
    int ret = COMPILE_OK;
    SymbolName name = handle_correct_queue->GetCurrentName();
    if (name == CONST_SYM) {
        if ((ret = m_constant_declaration.Generate()) == COMPILE_OK) {
            m_constant_declaration.LogOutput();
        }
    }
    if ((ret = m_variable_declaration.Generate()) == COMPILE_OK) {
        m_variable_declaration.LogOutput();
    }
    symbol_table_tree->UpgradeAddress();
    while (true) {
        handle_correct_queue->SetCacheLocate();
        if ((ret = m_function_definition.Generate()) == COMPILE_OK) {
            m_function_definition.LogOutput();
        } else {
            handle_correct_queue->SetCurrentLocate();
            break;
        }
    }
    if ((ret = m_main_function.Generate()) == COMPILE_OK) {
        m_main_function.LogOutput();
    }
    return ret;
}