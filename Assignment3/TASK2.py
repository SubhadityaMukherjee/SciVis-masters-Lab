
#%%
from sklearn import preprocessing
import tensorflow as tf
from PIL import Image
import numpy as np
import pickle
#%%
with open("./mlp_img/data.pkl", "rb") as f:
    data_loaded = pickle.load(f)

with open("./mlp_img/labels.pkl", "rb") as f:
    names_li = pickle.load(f)
    le = preprocessing.LabelEncoder()
    le.fit(names_li)
    labels_loaded = le.transform(names_li)

# %%
data_loaded.shape
#%%
labels_loaded.shape
#%%
from sklearn.model_selection import train_test_split
#%%

import keras
from keras import layers
from keras import regularizers
#%%

x_train, x_test, Y_train, Y_test = train_test_split(data_loaded, labels_loaded, test_size=0.33, random_state=42)
x_train = x_train.astype('float32') / 255.
x_test = x_test.astype('float32') / 255.
# x_train = x_train.reshape((len(x_train), np.prod(x_train.shape[1:])))
# x_test = x_test.reshape((len(x_test), np.prod(x_test.shape[1:])))
x_train = np.reshape(x_train, ( -1, 84, 1))
x_test = np.reshape(x_test, ( -1, 84, 1))
print(x_train.shape)
print(x_test.shape)
#%%

input_img = keras.Input(shape=(-1, x_train.shape[-2], 1))

x = layers.Conv2D(64, (3, 3), activation='relu', padding='same')(input_img)
# x = layers.MaxPooling2D((2, 2), padding='same')(x)
x = layers.Conv2D(64, (3, 3), activation='relu', padding='same', strides=2)(x)
# x = layers.MaxPooling2D((2, 2), padding='same')(x)
x = layers.Conv2D(64, (3, 3), activation='relu', padding='same', strides=2)(x)
# x = layers.MaxPooling2D((2, 2), padding='same')(x)
x = layers.Conv2D(64, (3, 3), activation='relu', padding='same', strides=2)(x)
encoded = layers.Dense(2 , activity_regularizer=regularizers.l1(10e-5))(x)

x = layers.Conv2D(64, (3, 3), activation='relu', padding='same')(encoded)
x = layers.UpSampling2D((2, 2))(x)
x = layers.Conv2D(64, (3, 3), activation='relu', padding='same')(x)
x = layers.UpSampling2D((2, 2))(x)
x = layers.Conv2D(64, (3, 3), activation='relu', strides = 2)(x)
# x = layers.UpSampling2D((2, 2))(x)

decoded = layers.Dense(x_train.shape[-1] , activity_regularizer=regularizers.l1(10e-5))(x)

autoencoder = keras.Model(input_img, decoded)
encoder = keras.Model(input_img, encoded)
#%%
encoded_input = keras.Input(shape=x_train.shape)
decoder_layer = autoencoder.layers[-1]
decoder = keras.Model(encoded_input, decoder_layer(encoded_input))

# %%

opt = tf.keras.optimizers.Adam(learning_rate=.0005)
autoencoder.compile(optimizer=opt, loss='mean_squared_error')
#%%
autoencoder.fit(x_train, x_train,
                epochs=10,
                batch_size=1,
                shuffle=True
                )
# %%
x_test_2 = np.resize(x_test, (1, 1,3, 296352))
x_test_2.shape
#%%
encoded_imgs = autoencoder.predict(x_test_2)
encoded_imgs=  encoded_imgs.reshape(1, x_train.shape[-1])
print(encoded_imgs.shape)