// Load the TCP Library
net = require('net');

var showTraffic = true;

// Keep track of the chat clients
var clients = [];

// Start a TCP Server
net.createServer(function (socket) {

  console.log("Connected -> " + socket.remoteAddress+ ":" + socket.remotePort);

  // Put this new client in the list
  clients.push(socket);

  // Handle incoming messages from clients.
  socket.on('data', function (data) {
    broadcast(data, socket);
  });

  // Remove the client from the list when it leaves
  socket.on('end', function () 
  {
    clients.splice(clients.indexOf(socket), 1);
  });
  
  socket.on('error', function () 
  {
    clients.splice(clients.indexOf(socket), 1);
  });
  
  // Send a message to all clients
  function broadcast(message, sender) 
  {
	if(showTraffic)
	{
		console.log(message.toString('utf8'));
	}
	
    clients.forEach(function (client) 
    {
      // Don't want to send it to sender
      if (client !== sender)
      {
        client.write(message);
      }
    });
  }

}).listen(7856);

// Put a friendly message on the terminal of the server.
console.log("MapSync UE4 Plugin server running at port 7856\n");
