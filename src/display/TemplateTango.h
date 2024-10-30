#ifndef TEMPLATETANGO_H
#define TEMPLATETANGO_H

#include <Arduino.h>
#include <map>
#include <regex>
#include <sstream>
#include <cmath>
#include <stack>

class TemplateTango {
public:
    /**
     * @brief Render the template string with the provided variables.
     * @param templateStr The template string containing placeholders for variables.
     * @param variables A map of variable names and their corresponding values.
     * @return A string with variables replaced by their corresponding values.
     */
    static String render(const String& templateStr, const std::map<String, String>& variables) {
        String result = templateStr;
        std::regex varRegex("\\{\\{(.*?)\\}\\}");
        std::smatch matches;
        std::string resultStd = result.c_str();  // Convert to std::string for regex

        while (std::regex_search(resultStd, matches, varRegex)) {
            std::string varExpression = matches[1].str();
            //Serial.printf("Found variable expression: %s\n", varExpression.c_str());
            // Evaluate the expression
            String evaluated = evaluateExpression(varExpression.c_str(), variables);
            resultStd.replace(matches.position(0), matches.length(0), evaluated.c_str());
        }
        result = resultStd.c_str();  // Convert back to String
        return result;
    }

private:
    /**
     * @brief Evaluate an expression by replacing variables and computing the result.
     * @param expression The expression to evaluate.
     * @param variables A map of variable names and their corresponding values.
     * @return The evaluated result as a string.
     */
    static String evaluateExpression(const String& expression, const std::map<String, String>& variables) {
        // Replace variables with their values
        std::string expr = expression.c_str();
        for (const auto& var : variables) {
            //Serial.printf("Replacing variable %s with value %s\n", var.first.c_str(), var.second.c_str());
            std::regex varRegex("\\b" + std::string(var.first.c_str()) + "\\b");
            expr = std::regex_replace(expr, varRegex, var.second.c_str());
        }

        // Handle string concatenation with **
        std::regex concatRegex(R"((\"[^\"]*\")\s*\*\*\s*(\"[^\"]*\"))");
        std::smatch concatMatch;
        while (std::regex_search(expr, concatMatch, concatRegex)) {
            std::string left = concatMatch[1].str();
            std::string right = concatMatch[2].str();
            left = left.substr(1, left.size() - 2);  // Remove quotes
            right = right.substr(1, right.size() - 2);  // Remove quotes
            std::string concatenated = "\"" + left + right + "\"";
            expr.replace(concatMatch.position(0), concatMatch.length(0), concatenated);
        }

        return expr.c_str();
    }
};

#endif // TEMPLATETANGO_H
