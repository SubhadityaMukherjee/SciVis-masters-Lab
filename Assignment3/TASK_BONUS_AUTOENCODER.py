
#%%
from sklearn import preprocessing
import tensorflow as tf
from PIL import Image
import numpy as np
import pickle
#%%
with open("./mlp_img/data_bonus.pkl", "rb") as f:
    data_loaded = pickle.load(f)

with open("./mlp_img/labels_bonus.pkl", "rb") as f:
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
print(x_train.shape, x_test.shape)
#%%

class Autoencoder():
    def __init__(self):
        self.img_rows = 50
        self.img_cols = 50
        self.channels = 1
        self.img_shape = (self.img_rows, self.img_cols, self.channels)
        
        optimizer =tf.keras.optimizers.Adam(lr=0.0005)
        
        self.autoencoder_model = self.build_model()
        self.autoencoder_model.compile(loss='mse', optimizer=optimizer)
        self.autoencoder_model.summary()
    
    def build_model(self):
        input_layer = tf.keras.Input(shape=self.img_shape)
        
        # encoder
        h = tf.keras.layers.Conv2D(64, (3, 3), activation='relu', padding='same')(input_layer)
        h = tf.keras.layers.MaxPooling2D((2, 2), padding='same')(h)
        h = tf.keras.layers.Conv2D(64, (3, 3), activation='relu', padding='same')(h)
        h = tf.keras.layers.MaxPooling2D((2, 2), padding='same')(h)
        h = tf.keras.layers.Conv2D(64, (3, 3), activation='relu', padding='same')(h)
        h = tf.keras.layers.MaxPooling2D((2, 2), padding='same')(h)
        h = tf.keras.layers.Conv2D(64, (3, 3), activation='relu', padding='same')(h)
        h = tf.keras.layers.MaxPooling2D((2, 2), padding='same')(h)
        h = tf.keras.layers.Dense(2 , activity_regularizer=regularizers.l1(10e-5))(h)
       
        # decoder

        h = tf.keras.layers.Dense(64)(h)
        # h = tf.keras.layers.Conv2D(64, (3, 3), activation='relu', padding='same')(h)
        h = tf.keras.layers.Conv2DTranspose(64, (3, 3), activation='relu', padding='same')(h)
        h = tf.keras.layers.Conv2DTranspose(64, (3, 3), activation='relu', padding='same')(h)
        h = tf.keras.layers.Conv2DTranspose(64, (3, 3), activation='relu', padding='same')(h)
        h = tf.keras.layers.Conv2DTranspose(64, (3, 3), activation='relu', padding='same')(h)

        h = tf.keras.layers.Dense(1)(h)
        # h = tf.keras.layers.Conv2D(64, (3, 3), activation='relu', padding='same')(h)

        # h = tf.keras.layers.Conv2D(64, (3, 3), activation='relu', padding='same')(h)
        # h = tf.keras.layers.UpSampling2D((2, 2))(h)
        # h = tf.keras.layers.Conv2D(64, (3, 3), activation='relu', padding='same')(h)
        # h = tf.keras.layers.UpSampling2D((2, 2))(h)
        # h = tf.keras.layers.Conv2D(64, (3, 3), activation='relu', padding='same')(h)
        # h = tf.keras.layers.UpSampling2D((2, 2))(h)

        # h = tf.keras.layers.Conv2D(1, (3, 3), activation='sigmoid', padding='same')(h)
        
        return tf.keras.Model(input_layer, h)
    
    def train_model(self, x_train, y_train, x_val, y_val, epochs, batch_size=20):
        tf.keras.utils.plot_model(self.autoencoder_model, to_file="outputs/model_shape.png", show_shapes=True)
        history = self.autoencoder_model.fit(x_train, y_train,
                                             batch_size=batch_size,
                                             epochs=epochs,
                                             validation_data=(x_val, y_val),)
    def eval_model(self, x_test):
        preds = self.autoencoder_model.predict(x_test)
        return preds
#%%
ae = Autoencoder()
ae.train_model(x_train, Y_train, x_test, Y_test, epochs=10, batch_size=1)

#%%
ae.eval_model(x_test).size
# %%
