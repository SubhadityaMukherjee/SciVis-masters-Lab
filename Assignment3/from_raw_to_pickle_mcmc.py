from matplotlib import pyplot as plt
import numpy as np
import time
import pickle

# data = np.empty((50,50,95000))

# data.data[:] = open('mcmc_a.raw').read()
# start_time = time.time()

data = np.fromfile('mcmc_a.raw', dtype='f8')
print(data.shape)
data.resize(95000, 50, 50)
print(data.shape)

# fig=plt.figure()
# columns = 10
# rows = 10
# for i in range(1, columns*rows+1 ):
#     if (i == data.shape[0]):
#         break
#     shift = 9100
#     img = data[i+shift,:,:]
#     # img = data[:,:,i+10000]
#     img = np.flip(img, 1)
#     #print(img.shape)
#     fig.add_subplot(rows, columns, i)
#     plt.imshow(img)
#     plt.axis('off')

# fig = plt.gcf()
# plt.suptitle(shift)
# plt.axis('off')
# plt.show()

examples = []
examples.append(data[5050])
examples.append(data[22050])
examples.append(data[13050])
examples.append(data[40050])
examples.append(data[7050])
examples.append(data[36050])
examples.append(data[18050])
examples.append(data[75050])
examples.append(data[32050])

fig=plt.figure()
columns = 9
rows = 1
for i in range(1, columns*rows+1 ):
    if (i == data.shape[0]):
        break
    img = examples[i-1]
    img = np.flip(img, 1)
    fig.add_subplot(rows, columns, i)
    # fig.tight_layout(pad=2.0)
    plt.imshow(img)
    plt.axis('off')

fig = plt.gcf()

# plt.suptitle(shift)
plt.axis('off')
plt.show()
fig.savefig('examples.png', dpi=300)

# input("waiting...")

labelled = False # True
if labelled:
    # save only labelled data
    labelled_data = np.zeros((0, 50, 50))
    names = []
    f = open("labels_mcmc.txt", 'r')
    for line in f:
        l = line.split(" ")
        # names.append(l[1][0])
        from_to = l[0].split("-")
        print(from_to[0], from_to[1])
        labelled_data = np.append(labelled_data, data[int(from_to[0]):int(from_to[1]),...], axis=0)
        for i in range(int(from_to[1])-int(from_to[0])):
            names.append(l[1][0])

    print(len(names))
    print(labelled_data.shape)

    # input("waiting...")

    pkl_file = open("mcmc_labelled_data.pkl", 'wb')
    pickle.dump(labelled_data, pkl_file, protocol=4)
    pkl_file.close

    pkl_file = open("mcmc_labelled_names.pkl", 'wb')
    pickle.dump(names, pkl_file, protocol=4)
    pkl_file.close
else: 
    # save all data
    # input("waiting...")

    pkl_file = open("mcmc.pkl", 'wb')
    pickle.dump(data, pkl_file, protocol=4)
    pkl_file.close

    pkl_file = open("mcmc.pkl", 'rb')
    data = []
    data = pickle.load(pkl_file)

    print(data.shape)

# elapsed_time = time.time() - start_time
# print("All", count, "frames were loaded successfully in", "{0:.2f}".format(round(elapsed_time, 2)), "seconds.")