import pandas as pd
import numpy as np 
from sklearn.model_selection import train_test_split
from sklearn.preprocessing import LabelEncoder
from sklearn.metrics import classification_report
from sklearn import datasets, svm
from sklearn import tree
from sklearn.naive_bayes import CategoricalNB
from sklearn.datasets import load_digits
from sklearn.naive_bayes import GaussianNB


# URL of the dataset
url = "https://archive.ics.uci.edu/ml/machine-learning-databases/mushroom/agaricus-lepiota.data"

# Column names as per the dataset description
columns = [
    "class", "cap-shape", "cap-surface", "cap-color", "bruises", "odor",
    "gill-attachment", "gill-spacing", "gill-size", "gill-color", "stalk-shape",
    "stalk-root", "stalk-surface-above-ring", "stalk-surface-below-ring",
    "stalk-color-above-ring", "stalk-color-below-ring", "veil-type", "veil-color",
    "ring-number", "ring-type", "spore-print-color", "population", "habitat"
]

# Load the dataset directly into memory
# data = pd.read_csv(url, header=None, names=columns)
# data = pd.read_csv(url, header=None, names=columns)
# data.to_csv('mushroom.csv', index=False)

data = pd.read_csv('mushroom.csv')

# Separate features (X) and target (y)
X = data.drop("class", axis=1)  # Features
y = data["class"]              # Target

# Split into training and testing sets
X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42)

# Cap-shape Naive Bayes Categorical from scratch 
X_train = pd.concat([X_train, y_train], axis='columns')

# put features (X) and target (y) back together 
X_train_cap = X_train[['cap-shape', 'class']]
classes = X_train_cap["class"].unique().tolist()  # Get unique class labels
cap_shapes = X_train_cap["cap-shape"].unique().tolist() # Get unique attribute values 

print('Classes', classes)
print('Cap Shapes', cap_shapes)

priors = {}
likelihoods = {}

# value_counts calculates how many instances of each value are present in the data
# with the normalize flag it can convert these value counts to probabilities 
priors = X_train_cap["class"].value_counts(normalize=True).to_dict()

# calculate the marginal probabilities for each cap-shape 
marginals = X_train_cap["cap-shape"].value_counts(normalize=True).to_dict()

# calculate the likelihoods for each class (edible and poisonous)
# Each likihood should be a dictionary with keys being the cap_shape and
# values being the corresponding probabilities
# Important note: some cap_shapes might not have an instance in a particular class
# in which case they will not be taken into account by value_counts. You will need
# to set those to 0. 

for c in classes:
    class_data = X_train_cap[X_train_cap["class"] == c]
    # YOUR CODE GOES HERE 
    cap_shapes_w_values = class_data["cap-shape"].value_counts(normalize=True).to_dict()

    likelihoods[c] = {}
    for cap_type in cap_shapes:
        if cap_type in cap_shapes_w_values:
            likelihoods[c][cap_type] = cap_shapes_w_values[cap_type]
        else:
            likelihoods[c][cap_type] = 0.0

print("Cap_Shapes_w_values ",cap_shapes_w_values)
print("Likelihoods ",likelihoods)
print("Priors", priors)
print("Marginals", marginals)


# calculate the posterior probabilities for each class based on cap_shape
# using Bayes Rule
# the posteriors should be a dictionary indexed by cap_shape with values
# being dictionaries indexed by class with the corresponding probabilities

posteriors = {}
for a in cap_shapes:     
    posteriors = {}
for a in cap_shapes:
    posteriors[a] = {}
    for c in classes:
        c_likelihood = likelihoods[c][a]
        prior = priors[c] 
        marginal = marginals[a]
        posteriors[a][c] = (c_likelihood * prior) / marginal

print("Posteriors", posteriors)


# cap-shape: bell=b,conical=c,convex=x,flat=f, knobbed=k,sunken=s

# Uncomment these when you have the calculations of all quantities working 

prior_edible = np.round(priors['e'],2)
posterior_edible_if_bell = (np.round(posteriors['b']['e'],2))
posterior_edible_if_flat = (np.round(posteriors['x']['e'],2))

print('Prior probability the mushroom is edible : ', prior_edible)
print('Posterior probability the mushroom is edible if it has a flat cap_shape :', posterior_edible_if_flat)
print('Posterior probability the mushroom is edible if it has a bell cap_shape :', posterior_edible_if_bell)

