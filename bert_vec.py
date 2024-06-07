from transformers import BertTokenizer, BertModel
import torch
import torch.nn as nn

# 加载BERT预训练模型和tokenizer
model_name = 'bert-base-chinese'
tokenizer = BertTokenizer.from_pretrained(model_name)
model = BertModel.from_pretrained(model_name)

def to_vec(text):
    # 定义调整向量长度的神经网络
        class VectorAdjustment(nn.Module):
            def __init__(self, input_size, output_size):
                super(VectorAdjustment, self).__init__()
                self.fc = nn.Linear(input_size, output_size)

            def forward(self, x):
                return self.fc(x)

    # 使用tokenizer将文本标记化为token序列


        tokens = tokenizer.tokenize(text)

        # 添加起始和结束标记
        tokens = ['[CLS]'] + tokens + ['[SEP]']

        # 将标记化的token转换为对应的id
        input_ids = tokenizer.convert_tokens_to_ids(tokens)

        # 分段处理的最大长度（BERT模型最大长度为512）
        max_segment_len = 512

        # 分割文本为多个段落
        segments = []
        current_segment = []
        for token in tokens:
            current_segment.append(token)
            if len(current_segment) == max_segment_len:
                segments.append(current_segment)
                current_segment = []
        if current_segment:
            segments.append(current_segment)

        # 创建PyTorch的tensor来存储BERT向量
        input_tensors = []
        for segment in segments:
            # 将标记转换为对应的id
            input_ids = tokenizer.convert_tokens_to_ids(segment)
            # 创建PyTorch的tensor
            input_tensor = torch.tensor([input_ids])
            input_tensors.append(input_tensor)

        # 获取文本的BERT向量表示
        with torch.no_grad():
            # 存储所有段落的向量表示
            all_embeddings = []
            for input_tensor in input_tensors:
                outputs = model(input_tensor)
                embeddings = outputs[0]
                all_embeddings.append(embeddings)

        # 合并所有段落的向量表示
        combined_embeddings = torch.cat(all_embeddings, dim=1)
        # 获取整个文本的向量表示（CLS对应的向量）
        text_embedding = combined_embeddings[0][0]
        # 定义并使用神经网络调整向量长度为64
        adjustment_net = VectorAdjustment(text_embedding.size(0), 64)
        adjusted_embedding = adjustment_net(text_embedding)
        return adjusted_embedding

# # 要转换的中文文本
# text_documents = []
# user_id=[]
# df=pd.read_csv("tresult.csv")
# words_list=[]
# # 构建user_id 对应的字典
# user_words={}
# for index,row in df.iterrows():
#     user_id.append(row[0])
#     text_documents.append(row[1])
#
# user_emb={}
# # Generate vectors for each document in the CSV file
# for id,text in zip(user_id,text_documents):
#     print(text)
#     user_emb[id + 9099] =to_vec(text.strip())
#
# # 保存字典
# # 保存文件
# with open("user_emb_bert.pkl", "wb") as tf:
#     pickle.dump(user_emb,tf)
