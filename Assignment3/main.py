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
import os

import torch
import torchvision
from torch import nn
from torch.autograd import Variable
from torch.utils.data import DataLoader, TensorDataset
from torchvision import transforms
from sklearn import preprocessing
from torchvision.utils import save_image
#%%
if not os.path.exists('./mlp_img'):
    os.mkdir('./mlp_img')
#%%
def to_img(x):
    x = 0.5 * (x + 1)
    x = x.clamp(0, 1)
    x = x.view(x.size(0), 1, 28, 28)
    return x
#%%

num_epochs = 1
batch_size = 128
learning_rate = 1e-3

img_transform = transforms.Compose([
    transforms.ToTensor(),
    transforms.Normalize([0.46, 0.48, 0.51], [0.32, 0.32, 0.32]),
])
#%%
print(data.shape)
names_li = [x.split(" ")[-1] for x in names]
tensor_x = torch.Tensor(data) # transform to torch tensor
le = preprocessing.LabelEncoder()
le.fit(names_li)
tensor_y = torch.Tensor(le.transform(names_li))
#%%
dataset = TensorDataset(tensor_x, tensor_y)
dataloader = DataLoader(dataset, batch_size=batch_size, shuffle=True)
#%%

class autoencoder(nn.Module):
    def __init__(self):
        super(autoencoder, self).__init__()
        self.encoder = nn.Sequential(
            nn.Conv2d(37044, 1024, 3, stride=3, padding=1),  # b, 16, 10, 10
            nn.ReLU(True),
            # nn.MaxPool2d(2, stride=2),  # b, 16, 5, 5
            nn.Conv2d(1024, 512, 3, stride=2, padding=1),  # b, 8, 3, 3
            nn.ReLU(True),
            nn.Conv2d(512, 256, 3, stride=2, padding=1),  # b, 8, 3, 3
            nn.ReLU(True),
            nn.Conv2d(256, 128, 3, stride=2, padding=1),  # b, 8, 3, 3
            nn.ReLU(True),


            # nn.MaxPool2d(2, stride=1)  # b, 8, 2, 2
        )
        self.decoder = nn.Sequential(
            nn.ConvTranspose2d(128, 512, stride=2),  # b, 16, 5, 5
            nn.ReLU(True),
            nn.ConvTranspose2d(512, 1024, stride=3, padding=1),  # b, 8, 15, 15
            nn.ReLU(True),
            nn.ConvTranspose2d(1024, 37044, stride=2, padding=1),  # b, 1, 28, 28
            nn.Tanh()
        )
    def forward(self, x):
        x = self.encoder(x)
        x = self.decoder(x)
        return x

#%%
model = autoencoder().cuda()
criterion = nn.MSELoss()
optimizer = torch.optim.Adam(
    model.parameters(), lr=learning_rate, weight_decay=1e-5)
#%%
for epoch in tqdm(range(num_epochs)):
    for data in dataloader:
        img, _ = data
        img = img.view(img.size(0), -1)
        img = Variable(img).cuda()
        # ===================forward=====================
        output = model(img)
        print(output.shape, img.shape)
        loss = criterion(output, img)
        # ===================backward====================
        optimizer.zero_grad()
        loss.backward()
        optimizer.step()
    # ===================log========================
    print('epoch [{}/{}], loss:{:.4f}'
          .format(epoch + 1, num_epochs, loss.data[0]))
    # if epoch % 10 == 0:
    #     pic = to_img(output.cpu().data)
    #     save_image(pic, './mlp_img/image_{}.png'.format(epoch))
