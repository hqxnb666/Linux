from graphviz import Digraph
import json

class FiniteAutomaton:
    def __init__(self):
        self.states = set()
        self.alphabet = set()
        self.transitions = {}
        self.startState = None
        self.acceptStates = set()

    def addState(self, state):
        self.states.add(state)

    def setStartState(self, state):
        self.startState = state

    def addAcceptState(self, state):
        self.acceptStates.add(state)

    def addTransition(self, start, inputSymbol, end):
        if start not in self.transitions:
            self.transitions[start] = []
        self.transitions[start].append((inputSymbol, end))
        self.alphabet.add(inputSymbol)

    def drawFA(self, outputFilename='finiteAutomaton', layout='dot'):
        dot = Digraph(comment='有限自动机', engine=layout)
        dot.attr('node', style='filled', fillcolor='white', fontname='Microsoft YaHei')
        dot.attr('edge', fontname='Microsoft YaHei')

        dot.node('start', shape='none', fontcolor='black')
        dot.edge('start', self.startState, label='Start', style='dashed')

        for state in self.states:
            shape = 'doublecircle' if state in self.acceptStates else 'circle'
            dot.node(state, state, shape=shape, height='0.5')

        for start, transitions in self.transitions.items():
            for inputSymbol, end in transitions:
                dot.edge(start, end, label=inputSymbol, fontsize='12', fontcolor='blue')

        dot.render(outputFilename, view=True, format='png')

    def faToRlg(self, interactive=False):
        rules = []
        print("开始将有限自动机转换为右线性文法...")
        for start, transitions in self.transitions.items():
            for inputSymbol, end in transitions:
                rule = f"{start} -> {inputSymbol}{end}"
                rules.append(rule)
                if interactive:
                    print(f"转换转移 {start} --{inputSymbol}--> {end} 为规则：{rule}")
                    input("按回车键继续...")
        for accept in self.acceptStates:
            if accept not in self.transitions or not any(transitions for transitions in self.transitions[accept]):
                rule = f"{accept} -> ε"
                rules.append(rule)
                if interactive:
                    print(f"为接受状态 {accept} 添加ε规则：{rule}")
                    input("按回车键继续...")
        return rules

    def printRlg(self, rules, toFile=False, filename='rlgOutput.txt'):
        if toFile:
            with open(filename, 'w', encoding='utf-8') as file:
                for rule in rules:
                    file.write(rule + '\n')
        else:
            print("右线性文法规则:")
            for rule in rules:
                print(rule)

def loadFaFromFile(filename):
    try:
        with open(filename, 'r', encoding='utf-8') as file:
            data = json.load(file)
            fa = FiniteAutomaton()
            for state in data['states']:
                fa.addState(state)
            fa.setStartState(data['startState'])
            for acceptState in data['acceptStates']:
                fa.addAcceptState(acceptState)
            for transition in data['transitions']:
                fa.addTransition(transition['start'], transition['inputSymbol'], transition['end'])
            return fa
    except FileNotFoundError:
        print("错误：文件未找到，请确保文件路径和名称正确无误。")
        exit()
    except json.JSONDecodeError:
        print("错误：JSON文件格式不正确，请检查文件内容。")
        exit()

def main():
    filename = input("请输入定义文件的名称（JSON格式）: ")
    fa = loadFaFromFile(filename)
    fa.drawFA()

    interactive = input("您希望逐步执行转换过程吗？（是/否）: ").lower() == '是'
    rlgRules = fa.faToRlg(interactive)

    outputChoice = input("您希望将 RLG 保存到文件吗？（是/否）: ").lower()
    if outputChoice == '是':
        outputFilename = input("请输入 RLG 输出文件的名称: ")
        fa.printRlg(rlgRules, toFile=True, filename=outputFilename)
    else:
        fa.printRlg(rlgRules)

if __name__ == '__main__':
    main()
