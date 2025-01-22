const express = require('express');
const jwt = require('jsonwebtoken');

const app = express();
app.use(express.json());

const SECRET_KEY = "supersecretkey";
const USER = "admin";
const PASSWORD = "password";

//login
app.post('/login', (request, result) => {
    const { username, password } = request.body;

    if (username === USER && password === PASSWORD) {
        // Generate a JWT token valid for 1 hour
        const token = jwt.sign({ username }, SECRET_KEY, { expiresIn: '1h' });
        return result.json({ token });
    }

    return result.status(401).json({ message: "Wrong credentials" });
});

function authenticateToken(request, result, next) {
    const token = request.headers['authorization'];

    try {
        const decoded = jwt.verify(token, SECRET_KEY);
        request.user = decoded;
        next();
    } catch (error) {
        if (error.name === 'TokenExpiredError') {
            return result.status(401).json({ message: "Token has expired" });
        } else {
            return result.status(401).json({ message: "Invalid token" });
        }
    }
}

//protected
app.get('/protected', authenticateToken, (request, result) => {
    result.json({ message: `Hello, ${request.user.username}! You have access to the protected route.` });
});

const PORT = 3000;
app.listen(PORT, () => {
    console.log(`Server is running on http://localhost:${PORT}`);
});