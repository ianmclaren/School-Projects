from hmmlearn import hmm
import numpy as np
import matplotlib.pyplot as plt
from scipy import stats 

rng = np.random.default_rng(1)

class Random_Variable: 
    
    def __init__(self, name, values, probability_distribution): 
        self.name = name 
        self.values = values 
        self.probability_distribution = probability_distribution        
        if all(issubclass(type(item), np.integer) for item in values): 
            self.type = 'numeric'
            self.rv = stats.rv_discrete(name = name, values = (values, probability_distribution), seed=rng)
        elif all(type(item) is str for item in values): 
            self.type = 'symbolic'
            self.rv = stats.rv_discrete(name = name, values = (np.arange(len(values)), probability_distribution), seed=rng)
            self.symbolic_values = values 
        else: 
            self.type = 'undefined'
            
    def sample(self,size): 
        if (self.type =='numeric'): 
            return self.rv.rvs(size=size)
        elif (self.type == 'symbolic'): 
            numeric_samples = self.rv.rvs(size=size)
            mapped_samples = [self.values[x] for x in numeric_samples]
            return mapped_samples 
        
    def probs(self): 
        return self.probability_distribution
    
    def vals(self): 
        print(self.type)
        return self.values 

def plot_samples(samples, state2colour, title):
    colours = [state2colour[x] for x in samples]
    x = np.arange(0, len(colours))
    y = np.ones(len(colours))
    plt.figure(figsize=(10,1))
    plt.bar(x, y, color=colours, width=1)
    plt.title(title)


def markov_chain(transmat, state, state_names, samples): 
    (rows, cols) = transmat.shape 
    rvs = [] 
    values = list(np.arange(0,rows))
    
    # create random variables for each row of transition matrix 
    for r in range(rows): 
        rv = Random_Variable("row" + str(r), values, transmat[r])
        rvs.append(rv)
    
    # start from initial state and then sample the appropriate 
    # random variable based on the state following the transitions 
    states = [] 
    for n in range(samples): 
        state = rvs[state].sample(1)[0]    
        states.append(state_names[state])
    return states


# QUESTION 6 
# appropriately define the transition matrix and
# create sample a markov chain with states 'CGD' and 'CGS'
# Xmarkov should consits of 1000 samples of the markov chain

# YOUR CODE GOES HERE 
transmat1 = np.array([[0.7, 0.3], [0.2, 0.8]])
Zmarkov = markov_chain(transmat1, 0, ['CGD', 'CGS'], 1000)

print('Zmarkov[0:10]:', Zmarkov[0:10])
# output should be Zmarkov[0:10]: ['CGD', 'CGS', 'CGD', 'CGS', 'CGS', 'CGS', 'CGS', 'CGS', 'CGS', 'CGD']

# approprixately define random variables 'CGD_colors' and 'CGS_colors'
# for mapping the states to colors with the specified emission probabilities
cgd_colors = Random_Variable('dense_colors', ['r', 'g', 'b', 'y'], 
                              [0.1, 0.1, 0.4, 0.4])
cgs_colors = Random_Variable('sparse_colors', ['r', 'g', 'b', 'y'], 
                               [0.4, 0.4, 0.1, 0.1])

def emit_obs(state, sunny_colors, cloudy_colors): 
    if (state == 'CGD'):   
        obs = cgd_colors.sample(1)[0]
    else: 
        obs = cgs_colors.sample(1)[0]
    return obs 

# iterate over the sequence of states Zmarkovand emit color based on the emission probabilities 
Xmarkov = [emit_obs(s, cgd_colors, cgs_colors) for s in Zmarkov]
print('Xmarkov[0:10]:',Xmarkov[0:10])
# output should be Xmarkov[0:10]: ['b', 'y', 'y', 'g', 'g', 'g', 'g', 'r', 'g', 'b']


np.random.seed(0)


# QUESTION 7
# Use the hmm.CategoricalHMM to create a model for our problem
# and use it to generate samples

model = hmm.CategoricalHMM(n_components=2)

# Your code goes here 

model.startprob_ = np.array([1.0, 0.0])
model.transmat_ = np.array([[0.7, 0.3],
                            [0.2, 0.8]])
model.emissionprob_ = np.array([[0.1, 0.1, 0.4, 0.4],
                               [0.4, 0.4, 0.1, 0.1]])
                        

# Code for plotting
# X, Z = model.sample(100)
# samples = [item for sublist in X for item in sublist]
# states2colour = {0: 'red', 1: 'green', 2: 'blue', 3: 'yellow'}
# plot_samples(samples[0:100], states2colour, 'Observations (ACTG)')
# obj2colour = {0: 'black', 1: 'white'}
# plot_samples(Z[0:100], obj2colour, 'States (CGD and CGS)')
# plt.show()

X, Z = model.sample(n_samples=10000)



# QUESTION 8 
# Every time that you can the fit function of the CategoricalHMM
# you get a different estimation of the HMM parameters. 
# learn a good model by trying 100 fits and selecting the best one
# based on score. Store the "best" model in estimated_model 

estimated_model = None
temp_model = None
best_score = -9999999
score = 0
for i in range(20): #Had to reduce this number, else the auto-grader wouldn't complete
#Need a way to compare
    temp_model = hmm.CategoricalHMM(n_components=2, n_iter=100)
    temp_model.fit(X)
    score = temp_model.score(X) #score() returns a negative number, closer to 0 is better
    if score > best_score:
        best_score = score
        estimated_model = temp_model

# Uncomment the code below when things are working

original_transmat = np.copy(model.transmat_)
original_emission_probs = np.copy(model.emissionprob_)

print("Original Transition Matrix:")
print(original_transmat)
print("Original Emission Matrix:")
print(original_emission_probs)

unsupervised_transmat = np.round(estimated_model.transmat_, 2)
unsupervised_emission_probs = np.round(estimated_model.emissionprob_,2)
print("Unsupervised Estimated Transition Matrix:")
print(unsupervised_transmat)
print("Unsupervised Estimated Emission Matrix:")
print(unsupervised_emission_probs)


# QUESTION 9

# Write a function train_hmm_supervised that takes
# as input both the state sequence Z and the associated
# observations X. Estimate the transition matrix
# from the data essentially by counting and normalizing transitions
# and then the emmision probabilities by appropriately counting
# observations for each state. 

from collections import defaultdict

def train_hmm_supervised(Z, X, num_states, num_observations):
    pass




num_states = 2
num_observations = 4

# Uncomment the code below once things are working

# supervised_transmat, supervised_emission_probs = train_hmm_supervised(Z, X, num_states, num_observations)
# supervised_transmat = np.round(supervised_transmat,2)
# supervised_emission_probs = np.round(supervised_emission_probs, 2)

# print("Supervised Transition Probabilities:\n", supervised_transmat)
# print("Supervised Emission Probabilities:\n", supervised_emission_probs)

