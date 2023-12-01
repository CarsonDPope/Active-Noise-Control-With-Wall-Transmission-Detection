# Convolutional Neural Network Training, and Analysis
# Author: Carson Pope
# Last Updated: 12/1/2023

import pandas as pd
import numpy as np
import tensorflow as tf
import keras
from tensorflow.keras.models import Sequential
from keras.layers import Dense, Dropout, Flatten, Conv1D, MaxPooling1D, Input, Activation, BatchNormalization
from sklearn.model_selection import train_test_split
from keras.initializers import RandomNormal
from tensorflow.keras.optimizers import Adam
import matplotlib.pyplot as plt
from sklearn.metrics import classification_report, accuracy_score, confusion_matrix
import cv2

# Read in csv file (must execute broadbandGen.py first)
InputSamples = pd.read_csv('CapstoneMachineLearningTrainingInputs.csv')

Inputs = InputSamples.iloc[1:]
Outputs = InputSamples.iloc[0]


# Define, train and fit model
# Defined model
def create_cnn(numSamples, numOutputs, filters=20, kernel_size=50, regress=False):
    
    model = Sequential([
        Conv1D(filters, kernel_size, activation='relu', input_shape=(numSamples,1)),
        Conv1D(filters, kernel_size, activation='relu'),
        MaxPooling1D(pool_size=10),
        Dropout(0.25),
        Flatten(),
        Dense(15, activation='relu'),
        Dense(numOutputs, activation='softmax')])
        
    # return the CNN
    return model

# print(Inputs.shape[1]);
# print(len(Outputs));

# Test split
X_train, X_test, y_train, y_test = train_test_split(Inputs.transpose(), Outputs.transpose(), test_size=0.25, random_state=42)

# number of filters (secondary plants)
numPlants = 3;

y_train = tf.keras.utils.to_categorical(y_train, numPlants)
y_test = tf.keras.utils.to_categorical(y_test, numPlants)

# Create cnn
model = create_cnn(X_train.shape[1], numPlants, 50, 30, True)
opt = Adam()
model.compile(loss="binary_crossentropy", optimizer=opt)

# Fit model
print(X_train.shape)
print("[INFO] training model...")
model.fit(x=X_train, y=y_train, 
    validation_data=(X_test,y_test),
    epochs=25, batch_size=10)

model.summary()

model.save('Capstone.keras')  # The file needs to end with the .keras extension

#Training Accuracy
y_train_pred = model.predict(X_train)
predict_class = np.argmax(np.array(y_train_pred), axis=1)
predict_class = predict_class.tolist()

train_accuracy= accuracy_score(y_train_pred.argmax(axis=1), predict_class)
print("Training accuracy: ", train_accuracy)

#Testing Accuracy
y_pred = model.predict(X_test)
predict_class = np.argmax(np.array(y_pred), axis=1)
predict_class = predict_class.tolist()

test_accuracy= accuracy_score(y_test.argmax(axis=1), predict_class)
print("Testing accuracy: ", test_accuracy)

# Show Confusion Matrix
cm = confusion_matrix(y_test.argmax(axis=1),predict_class)
print(cm)

tf.keras.utils.plot_model(
    model,
    to_file="model.png",
    show_shapes=False,
    show_dtype=False,
    show_layer_names=False,
    rankdir="LR",
    expand_nested=False,
    dpi=96,
    layer_range=None,
    show_layer_activations=False,
)
