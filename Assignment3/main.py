#%%
from numpy.linalg import norm
from tqdm import tqdm
from numpy.random.mtrand import normal
from labeled_data_pickle_flow import main as lbm
from unlabeled_data_pickle_flow import main as ulbm
from preprocessing import *
import numpy as np
from sklearn.decomposition import PCA

#%%

# Labelled data load
data, names = lbm()
print(data.shape, len(names))
#%%

normalized_cropped_data, data_mean, data_std = preprocess(data, True)
#%%
print(normalized_cropped_data.shape)
reshaped = np.einsum('ijkl->ij', normalized_cropped_data)
print(reshaped.shape)
X_train_pca = reshaped.copy()
#%%
names[:2]
len(names), reshaped.shape
# %%
pca = PCA(2)
pca_result = pca.fit_transform(reshaped)
pca1 = pca_result[:,0]
pca2 = pca_result[:,1] 
# pca3 = pca_result[:,2]
print('Explained variation per principal component: {}'.format(pca.explained_variance_ratio_))
#%%
# %%
import seaborn as sns
import pandas as pd

# %%
pca1.shape
#%%
X_train_pca = pd.DataFrame()
X_train_pca["pca-one"] = pca1
X_train_pca["pca-two"] = pca2
# X_train_pca["pca-three"] = pca3
X_train_pca["y"] = [x.split(" ")[-1] for x in names]

X_train_pca.head(3)
# %%
plt.figure(figsize=(16,10))
rndperm = np.random.permutation(X_train_pca.shape[0])
sns.scatterplot(
    x="pca-one", y="pca-two",
    hue="y",
    # palette=sns.color_palette("hls", 5),
    data=X_train_pca.loc[rndperm,:],
    legend="full",
    # alpha=0.3
)
plt.savefig("./outputs/pca.png")
#%%
# TSNE

from sklearn.manifold import TSNE

tsne = TSNE(n_components=2)
projections = tsne.fit_transform(reshaped)
#%%
proj = pd.DataFrame.from_records(projections)
#%%

plt.figure(figsize=(16,10))
sns.scatterplot(
    x=0, y=1,
    hue =[x.split(" ")[-1] for x in names],
    data = proj,
    legend="full",
    alpha=0.3
)

plt.savefig("./outputs/tsne.png")
#%%
# UMAP

import umap
um = umap.UMAP()
projections = um.fit_transform(reshaped)
#%%
proj = pd.DataFrame.from_records(projections)
#%%

plt.figure(figsize=(16,10))
sns.scatterplot(
    x=0, y=1,
    data = proj,

    hue =[x.split(" ")[-1] for x in names],
    legend="full",
    alpha=0.3
)

plt.savefig("./outputs/umap.png")
#%%
# Save data
with open("./mlp_img/data.pkl", "wb+") as f:
    pickle.dump(data, f)
with open("./mlp_img/labels.pkl", "wb+") as f:
    pickle.dump([x.split(" ")[-1] for x in names], f)