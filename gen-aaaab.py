import random

random.seed(1)
print(''.join(random.choices("ab", weights=[5000, 1], k=10 ** 6)))
