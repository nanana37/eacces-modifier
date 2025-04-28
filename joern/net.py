import networkx as nx
import json
import matplotlib.pyplot as plt  # グラフ描画用

# JSONファイルを読み込む
with open("slices.json", "r") as file:
    data = json.load(file)

# グラフの作成
G = nx.DiGraph()  # 有向グラフを使用

# ノードを追加
for node in data["nodes"]:
    node_id = node["id"]
    label_parts = [f"{value}" for key, value in node.items() if key != "id"]
    label = "\n".join(label_parts)
    G.add_node(node_id, label=label)

# エッジを追加
for edge in data["edges"]:
    src = edge["src"]
    dst = edge["dst"]
    label = edge["label"]
    G.add_edge(src, dst, label=label)
pos = nx.spring_layout(G, k=3.0, iterations=200)
edge_labels = nx.get_edge_attributes(G, 'label')
node_labels = {node: data["label"] for node, data in G.nodes(data=True)}

nx.draw_networkx_nodes(G, pos, node_color='lightgreen', node_size=3000, node_shape='s')
nx.draw(G, pos, labels=node_labels, with_labels=True, nodelist=[], font_size=6, arrows=True)
nx.draw_networkx_edge_labels(G, pos, edge_labels=edge_labels, font_color='red', font_size=5)

# グラフをファイルに保存
plt.savefig("graph_visualization.svg")
plt.show()
