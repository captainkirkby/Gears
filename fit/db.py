from pymongo import MongoClient
from pymongo import DESCENDING
import numpy

class DB(object):
    def __init__(self,args):
        # Set fetch constants
        self.ID              = u"_id"
        self.RAW             = u"raw"
        self.TIMESTAMP       = u"timestamp"
        self.CRUDE_PERIOD    = u"crudePeriod"
        self.TEMPLATE        = u"template"

        self.args = args
        # Connect to Mongod (fails and exits if mongod is not running)
        self.client = MongoClient('localhost', args.mongo_port)
        # Get db object
        self.db = self.client[args.db_name]
        # Get collection object
        self.dataCollection = self.db[args.collection_name]
        self.templateCollection = self.db[self.args.template_collection]

    def getNumTemplates(self):
        return self.templateCollection.count()

    # We DONT need the crude period data to generate a template
    def loadData(self):
        """
        Loads IR data from the database into the expected numpy format
        Crude Period        EDIT 3/27: this is set to zero breaking the --from-db option!
        IR
        IR...
        Returns the tuple (data,timestamp)
        """
        # Perform Query
        results = self.dataCollection.find({},{self.ID:False,self.TIMESTAMP:True, self.CRUDE_PERIOD:True, 
            self.RAW:True}).sort(self.TIMESTAMP, DESCENDING).limit(self.args.fetch_limit)
        # Construct numpy array
        data = numpy.array([], dtype=numpy.uint16)
        timestamp = 0
        for document in results:
            # data = numpy.append(data,document[self.CRUDE_PERIOD])
            data = numpy.append(data,0)
            data = numpy.append(data,document[self.RAW])
            # Get last timestamp
            timestamp = document[self.TIMESTAMP]
        return (data,timestamp)

    def saveTemplate(self,template,timestamp):
        """
        Saves a given template into the database
        """
        self.templateCollection.insert({self.TIMESTAMP:timestamp, self.TEMPLATE:template.T.tolist()})
        if self.args.verbose:
            print "Template Updated!"
        if self.args.show_plots:
            plt.plot(template[0],template[1])
            plt.draw()

    def loadTemplate(self):
        """
        Loads a given template from the database, returns the tuple (template,timestamp)
        """
        # Perform Query
        results = self.templateCollection.find({},{self.ID:False,self.TIMESTAMP:True, 
            self.TEMPLATE:True}).sort(self.TIMESTAMP, DESCENDING).limit(1)

        data = []
        templateTimestamp = None
        for document in results:
            templateTimestamp = document[self.TIMESTAMP]
            for pair in document[self.TEMPLATE]:
                data.append(pair)
        return (numpy.transpose(numpy.array(data)),templateTimestamp)

if __name__ == "__main__":
    main()
