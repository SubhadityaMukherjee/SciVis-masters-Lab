
# class autoencoder(nn.Module):
#     def __init__(self):
#         super(autoencoder, self).__init__()
#         
#     def forward(self, x):
#         x = self.encoder(x)
#         x = self.decoder(x)
#         return x
#%%
from sklearn import preprocessing
from PIL import Image
import torchvision
import pickle
import torch
#%%
with open("./mlp_img/data.pkl", "rb") as f:
    data_loaded = torch.Tensor(pickle.load(f))

with open("./mlp_img/labels.pkl", "rb") as f:
    names_li = pickle.load(f)
    le = preprocessing.LabelEncoder()
    le.fit(names_li)
    labels_loaded = torch.Tensor(le.transform(names_li))

# %%
data_loaded.shape
#%%
labels_loaded.shape
#%%
from pl_bolts.models.autoencoders import AE
from typing import Optional, Tuple
import torch
import torch.nn.functional as F
from torch import nn
from torch.utils.data import DataLoader, random_split

import pytorch_lightning as pl
from pytorch_lightning.utilities import rank_zero_only
from pytorch_lightning.utilities.cli import LightningCLI
import torchvision
from torchvision import transforms
from torchvision.utils import save_image
from torch.utils.data import TensorDataset

# %%
class ImageSampler(pl.callbacks.Callback):
    def __init__(
        self,
        num_samples: int = 3,
        nrow: int = 8,
        padding: int = 2,
        normalize: bool = True,
        norm_range: Optional[Tuple[int, int]] = None,
        scale_each: bool = False,
        pad_value: int = 0,
    ) -> None:
        """
        Args:
            num_samples: Number of images displayed in the grid. Default: ``3``.
            nrow: Number of images displayed in each row of the grid.
                The final grid size is ``(B / nrow, nrow)``. Default: ``8``.
            padding: Amount of padding. Default: ``2``.
            normalize: If ``True``, shift the image to the range (0, 1),
                by the min and max values specified by :attr:`range`. Default: ``False``.
            norm_range: Tuple (min, max) where min and max are numbers,
                then these numbers are used to normalize the image. By default, min and max
                are computed from the tensor.
            scale_each: If ``True``, scale each image in the batch of
                images separately rather than the (min, max) over all images. Default: ``False``.
            pad_value: Value for the padded pixels. Default: ``0``.
        """

        super().__init__()
        self.num_samples = num_samples
        self.nrow = nrow
        self.padding = padding
        self.normalize = normalize
        self.norm_range = norm_range
        self.scale_each = scale_each
        self.pad_value = pad_value

    def _to_grid(self, images):
        return torchvision.utils.make_grid(
            tensor=images,
            nrow=self.nrow,
            padding=self.padding,
            normalize=self.normalize,
            range=self.norm_range,
            scale_each=self.scale_each,
            pad_value=self.pad_value,
        )

    @rank_zero_only
    def on_epoch_end(self, trainer: pl.Trainer, pl_module: pl.LightningModule) -> None:

        images, _ = next(iter(DataLoader(trainer.datamodule.mnist_val, batch_size=self.num_samples)))
        images_flattened = images.view(images.size(0), -1)

        # generate images
        with torch.no_grad():
            pl_module.eval()
            images_generated = pl_module(images_flattened.to(pl_module.device))
            pl_module.train()

        if trainer.current_epoch == 0:
            save_image(self._to_grid(images), f"grid_ori_{trainer.current_epoch}.png")
        save_image(self._to_grid(images_generated.reshape(images.shape)), f"grid_generated_{trainer.current_epoch}.png")

#%%
class LitAutoEncoder(pl.LightningModule):
    """
    >>> LitAutoEncoder()  # doctest: +ELLIPSIS +NORMALIZE_WHITESPACE
    LitAutoEncoder(
      (encoder): ...
      (decoder): ...
    )
    """

    def __init__(self, hidden_dim: int = 64):
        super().__init__()
        self.encoder = nn.Sequential(
            nn.Conv2d(441, 1024,kernel_size= 3, stride=3, padding=1),  # b, 16, 10, 10
            nn.ReLU(True),
            nn.Conv2d(1024, 512, kernel_size= 3,stride=2, padding=1),  # b, 8, 3, 3
            nn.ReLU(True),
            nn.Conv2d(512, 256, kernel_size = 3, stride=2, padding=1),  # b, 8, 3, 3
            nn.ReLU(True),
            nn.Conv2d(256, 128, kernel_size=3, stride=2, padding=1),  # b, 8, 3, 3
            nn.ReLU(True),


            # nn.MaxPool2d(2, stride=1)  # b, 8, 2, 2
        )
        self.decoder = nn.Sequential(
            nn.ConvTranspose2d(128, 512,kernel_size=3, stride=2),  # b, 16, 5, 5
            nn.ReLU(True),
            nn.ConvTranspose2d(512, 1024,kernel_size=3, stride=3, padding=1),  # b, 8, 15, 15
            nn.ReLU(True),
            nn.ConvTranspose2d(1024, 441, kernel_size=3, stride=2, padding=1),  # b, 1, 28, 28
            nn.Tanh()
        )

    def forward(self, x):
        z = self.encoder(x)
        x_hat = self.decoder(z)
        return x_hat

    def training_step(self, batch, batch_idx):
        return self._common_step(batch, batch_idx, "train")

    def validation_step(self, batch, batch_idx):
        self._common_step(batch, batch_idx, "val")

    def test_step(self, batch, batch_idx):
        self._common_step(batch, batch_idx, "test")

    def predict_step(self, batch, batch_idx, dataloader_idx=None):
        x = self._prepare_batch(batch)
        return self(x)

    def configure_optimizers(self):
        optimizer = torch.optim.Adam(self.parameters(), lr=1e-3)
        return optimizer

    def _prepare_batch(self, batch):
        x, _ = batch
        return x
        # return x.view(x.size(0), -1)

    def _common_step(self, batch, batch_idx, stage: str):
        x = self._prepare_batch(batch)
        loss = F.mse_loss(x, self(x))
        self.log(f"{stage}_loss", loss, on_step=True)
        return loss

#%%
class MyDataModule(pl.LightningDataModule):
    def __init__(self, batch_size: int = 32):
        super().__init__()
        dataset = TensorDataset(data_loaded, labels_loaded)
        self.mnist_train, self.mnist_val = random_split(dataset, [29,7])
        self.batch_size = batch_size

    def train_dataloader(self):
        return DataLoader(self.mnist_train, batch_size=self.batch_size)

    def val_dataloader(self):
        return DataLoader(self.mnist_val, batch_size=self.batch_size)

    def test_dataloader(self):
        return DataLoader(self.mnist_test, batch_size=self.batch_size)

    def predict_dataloader(self):
        return DataLoader(self.mnist_test, batch_size=self.batch_size)

#%%
dm = MyDataModule()
model = LitAutoEncoder()
#%%
trainer = pl.Trainer()
trainer.fit(model, datamodule=dm)
#%%
trainer.validate(datamodule=dm)
# %%
