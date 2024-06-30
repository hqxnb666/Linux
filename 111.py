import pandas as pd
import numpy as np
import statsmodels.api as sm
import sklearn.model_selection as ms
from sklearn.metrics import mean_squared_error

# 数据加载
df = pd.read_csv('"C:\Users\29466\Documents\WeChat Files\wxid_otj11rla13v622\FileStorage\File\2024-04\abalone.csv"')  # 请确保文件路径正确
# 将DataFrame转换为numpy数组
df = df.values

# 数据预处理
y = df[:, -1].reshape(-1, 1)  # 提取rings列作为y，并转换为列向量
x = df[:, 1:8]  # 提取1-8列作为x
sex = df[:, [0]]  # 提取性别列

# 将性别转换为哑变量
male = (sex == 1).astype(int)  # 雄性为1，其他为0
female = (sex == -1).astype(int)  # 雌性为1，其他为0

# 合并x和性别哑变量
x = np.hstack((male, female, x))

# 划分训练集和测试集
x_train, x_test, y_train, y_test = ms.train_test_split(x, y, test_size=0.3, random_state=2021)

# 添加常数列以适应线性回归模型
xx_train = sm.add_constant(x_train)
xx_test = sm.add_constant(x_test)

# 建模
model = sm.OLS(y_train, xx_train).fit()

# 输出模型摘要
print(model.summary())

# 输出回归系数
print("回归系数：", model.params)

# 预测
y_pred = model.predict(xx_test)

# 计算并输出R2
print("R^2：", model.rsquared)

# 计算并输出测试集的MSE
mse = mean_squared_error(y_test, y_pred)
print("测试集MSE：", mse)
