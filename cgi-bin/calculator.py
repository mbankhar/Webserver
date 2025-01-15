#!/usr/bin/python3

import cgi
import cgitb

def calculate(num1, num2, operator):
    try:
        num1 = float(num1)
        num2 = float(num2)

        if operator == '+':
            return num1 + num2
        elif operator == '-':
            return num1 - num2
        elif operator == '*':
            return num1 * num2
        elif operator == '/':
            if num2 == 0:
                return "Error: Division by zero"
            return num1 / num2
        else:
            return "Error: Unsupported operator"
    except ValueError:
        return "Error: Invalid input"

# Output HTTP headers
print("Content-Type: text/html")
print()

# Parse input from query parameters
form = cgi.FieldStorage()
num1 = form.getvalue('num1')
num2 = form.getvalue('num2')
operator = form.getvalue('op')

# Validate inputs
if num1 is None or num2 is None or operator is None:
    print("<html><body>")
    print("<h1>Error: Missing parameters</h1>")
    print("<p>Provide parameters: num1, num2, and op (+, -, *, /)</p>")
    print("</body></html>")
else:
    # Perform the calculation
    result = calculate(num1, num2, operator)

    # Return the result as an HTML response
    print("<html><body>")
    print("<h1>Calculator Result</h1>")
    print(f"<p>{num1} {operator} {num2} = {result}</p>")
    print("</body></html>")