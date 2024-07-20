import pandas as pd
from datetime import datetime

class ForexTradingContract:
    def __init__(self):
        self.accounts = {}
        self.transaction_history = pd.DataFrame(columns=["日期", "账户名", "交易方向", "交易金额", "交易类型", "成交汇率", "交易费用"])

    def initialize_account(self, account_name, cny_balance, usd_balance):
        self.accounts[account_name] = {'CNY': cny_balance, 'USD': usd_balance}
        
    def execute_trade(self, date, account_name, direction, amount, trade_type, current_rate):
        if account_name not in self.accounts:
            return "账户不存在"

        amount = float(amount.replace('$', '').replace('￥', ''))
        if '买入' in direction:
            if '美元' in direction:
                required_cny = amount * current_rate
                fee = required_cny * 0.005
                if self.accounts[account_name]['CNY'] >= required_cny + fee:
                    self.accounts[account_name]['CNY'] -= (required_cny + fee)
                    self.accounts[account_name]['USD'] += amount
                    self.record_transaction(date, account_name, direction, amount, trade_type, current_rate, fee)
                else:
                    return "余额不足"
            else:
                required_usd = amount / current_rate
                fee = required_usd * 0.005
                if self.accounts[account_name]['USD'] >= required_usd + fee:
                    self.accounts[account_name]['USD'] -= (required_usd + fee)
                    self.accounts[account_name]['CNY'] += amount
                    self.record_transaction(date, account_name, direction, amount, trade_type, current_rate, fee)
                else:
                    return "余额不足"
        else:
            if '美元' in direction:
                fee = amount * 0.003
                if self.accounts[account_name]['USD'] >= amount + fee:
                    self.accounts[account_name]['USD'] -= (amount + fee)
                    self.accounts[account_name]['CNY'] += amount * current_rate
                    self.record_transaction(date, account_name, direction, amount, trade_type, current_rate, fee)
                else:
                    return "余额不足"
            else:
                fee = amount * 0.003
                if self.accounts[account_name]['CNY'] >= amount + fee:
                    self.accounts[account_name]['CNY'] -= (amount + fee)
                    self.accounts[account_name]['USD'] += amount / current_rate
                    self.record_transaction(date, account_name, direction, amount, trade_type, current_rate, fee)
                else:
                    return "余额不足"

    def record_transaction(self, date, account_name, direction, amount, trade_type, rate, fee):
        new_entry = {
            "日期": date,
            "账户名": account_name,
            "交易方向": direction,
            "交易金额": amount,
            "交易类型": trade_type,
            "成交汇率": rate,
            "交易费用": fee
        }
        self.transaction_history = self.transaction_history.append(new_entry, ignore_index=True)

    def check_balance(self, account_name):
        return self.accounts[account_name]

    def get_transaction_history(self):
        return self.transaction_history
