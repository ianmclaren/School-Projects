# CSC421 ASSIGNMENT 2 - QUESTION 1

def evaluate(s):
    operands = s.split()
    #print(f'operands: {operands}')
    operator = operands.pop(0)
    operands.sort() #operands sorted in alphabetical order
    if operator == '&':
        return operands[1] #The second value is Z if there is a Z, else U, else O
    elif operator == '|':
        return operands[0] #The first value is O if there is an O, else U, else Z
    return "Failure"

# examples
e1_1 = "& Z O"
e1_2 = "| O O"
e1_3 = "| Z Z"
e1_4 = "& U U"
e1_5 = "& U Z"

res_e1_1 = evaluate(e1_1)
res_e1_2 = evaluate(e1_2)
res_e1_3 = evaluate(e1_3)

#Added test cases
res_e1_4 = evaluate(e1_4)
res_e1_5 = evaluate(e1_5)


# print(f'{e1_1} = {res_e1_1}')
# print(f'{e1_2} = {res_e1_2}')
# print(f'{e1_3} = {res_e1_3}')

# #Added
# print(f'{e1_4} = {res_e1_4}')
# print(f'{e1_5} = {res_e1_5}')


# CSC421 ASSIGNMENT 2 - QUESTION 2

d = {'foo': "Z", 'b': "O"}
# print(d)
e2_1 = '& Z O'
e2_2 = '& foo O'
e2_3 = '& foo b'

#Added test cases
e2_4 = '& b O'
e2_5 = '| b U'

def handle_bindings(operand, bindings):
    set_variables = ["O", "U", "Z"]
    if operand not in set_variables:
        return bindings[operand]
    return operand

def evaluate_with_bindings(s,d):
    operands = s.split()
    operator = operands.pop(0)
    operands.sort()
    if operator == '&':
        return handle_bindings(operands[1], d)
    elif operator == '|':
        return handle_bindings(operands[0], d)
    return "Failure"


res_e2_1 = evaluate_with_bindings(e2_1,d)
res_e2_2 = evaluate_with_bindings(e2_2,d)
res_e2_3 = evaluate_with_bindings(e2_3,d)

#Added
res_e2_4 = evaluate_with_bindings(e2_4, d)
res_e2_5 = evaluate_with_bindings(e2_5, d)

# print(f'{e2_1} = {res_e2_1}')
# print(f'{e2_2} = {res_e2_2}')
# print(f'{e2_3} = {res_e2_3}')

# #Added
# print(f'{e2_4} = {res_e2_4}')
# print(f'{e2_5} = {res_e2_5}')


# CSC421 ASSIGNMENT 2 - QUESTIONS 3,4
def recursive_eval(list, dict):
    head, tail = list[0], list[1:]
    if head in ['&', '|']:
        val1, tail = recursive_eval(tail, dict)
        val2, tail = recursive_eval(tail, dict)
        operands = [handle_bindings(val1, dict), handle_bindings(val2, dict)]
        operands.sort()
        if head == '&':
            return (handle_bindings(operands[1], dict), tail) #This is wrong - operands aren't sorted
        elif head == '|':
            return (handle_bindings(operands[0], dict), tail)
    else:
        return (head, tail)


def prefix_eval(input_str, d):
    input_list = input_str.split(' ')
    res, tail = recursive_eval(input_list, d)
    return res 

d = {'a': 'O', 'b': 'Z', 'c': 'U'}
e3_1 = "& a | Z O"
e3_2 = "& O | O b"

#Added
e3_a = "| a | U O"
e3_b1 = "| c Z"
e3_b = "| c & O Z"

e3_3 = "| O & ~ b b"
e3_4 = "& ~ a & O O"
e3_5 = "| O & ~ b c"
e3_6 = "& ~ a & c O"
e3_7 = "& & c c & c c"

print(d)
for e in [e3_1,e3_2, e3_a, e3_b1, e3_b, e3_3,e3_4,e3_5,e3_6, e3_7]:
    print("%s \t = %s" % (e, prefix_eval(e,d)))

# EXPECTED OUTPUT
# & Z O = Z
# | O O = Z
#| Z Z = Z
# {'foo': 'Z', 'b': 'O'}
# & Z O = Z
# & foo O = Z
# & foo b = Z
# {'a': 'O', 'b': 'Z', 'c': 'U'}
# & a | Z O        = O
# & O | O b        = O
# | O & ~ b b      = O
# & ~ a & O O      = Z
# | O & ~ b c      = O
# & ~ a & c O      = Z
# & & c c & c c    = U
    




